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
 *  \package{Built-In Math Procedures}
 *
 *  This package contains the math built-in functions.  For the most
 *  complicated of these we simply call the standard C functions which do
 *  the same thing.
\*/

/* standard C header files */

#include <math.h>                      /* C math functions                  */
#if UNIX | MACINTOSH | WATCOM
#include <errno.h>                     /* error symbols                     */
#endif
#if VMS
#include errno                         /* error codes                       */
#endif

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "builtins.h"                  /* built-in symbols                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_reals.h"                   /* real numbers                      */
#include "x_strngs.h"                  /* strings                           */

#if !(WATCOM)
extern int errno;                      /* error flag                        */
#endif

/*\
 *  \function{setl2\_abs()}
 *
 *  This function is the \verb"abs" built-in function.  We have to go to
 *  a little more work than one might expect for integers.  This is
 *  because we accept different magnitudes for positive and negative
 *  short integers.
\*/

void setl2_abs(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
integer_h_ptr_type integer_hdr;        /* integer header pointer            */
int32 short_value;                     /* short integer value               */
int32 short_hi_bits;                   /* high order bits of short          */
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
string_h_ptr_type string_hdr;          /* string header pointer             */
string_c_ptr_type string_cell;         /* string cell pointer               */

   /* abs is valid for integers and reals */

   switch (argv[0].sp_form) {

      /*
       *  If the source is short, we find the absolute value and test
       *  whether it remains short.  If not we convert to long.
       */

      case ft_short :

         short_value = labs(argv[0].sp_val.sp_short_value);

         /* check whether the result remains short */

         if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
             short_hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_value;

            return;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_value);

         return;

      /*
       *  If the source is long, we find the absolute value and try to
       *  make it into a short.  This is necessary since the acceptable
       *  magnitudes of positive and negative shorts are not the same.
       */

      case ft_long :

         integer_hdr = argv[0].sp_val.sp_long_ptr;

         /* try to convert to short */

         if (integer_hdr->i_cell_count < 3) {

            short_value = labs(long_to_short(SETL_SYSTEM integer_hdr));

            /* check whether it will fit in a short */

            if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
                short_hi_bits == INT_HIGH_BITS) {

               unmark_specifier(target);
               target->sp_form = ft_short;
               target->sp_val.sp_short_value = short_value;

               return;

            }
         }

         /*
          *  We just make the sign of the long integer positive.  Notice
          *  that we don't try to use the argument destructively.
          */

         integer_hdr = copy_integer(SETL_SYSTEM integer_hdr);
         integer_hdr->i_is_negative = NO;
         unmark_specifier(target);
         target->sp_form = ft_long;
         target->sp_val.sp_long_ptr = integer_hdr;

         return;

      /*
       *  If the source is real, we copy the source to the target and
       *  form the absolute value.
       */

      case ft_real :

         real_number = fabs((argv[0].sp_val.sp_real_ptr)->r_value);

         /* allocate a real */

         unmark_specifier(target);
         i_get_real(real_ptr);
         target->sp_form = ft_real;
         target->sp_val.sp_real_ptr = real_ptr;
         real_ptr->r_use_count = 1;
         real_ptr->r_value = real_number;

         return;

      /*
       *  The abs function when applied to strings returns the numeric
       *  representation of the string.  This is machine dependent, but
       *  will probably be either the ASCII or EBCDIC value for any
       *  machine we support.
       */

      case ft_string :

         string_hdr = argv[0].sp_val.sp_string_ptr;

         if (string_hdr->s_length != 1)
            abend(SETL_SYSTEM msg_abs_too_long,abend_opnd_str(SETL_SYSTEM argv));

         string_cell = string_hdr->s_head;

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value =
            (unsigned char)(string_cell->s_cell_value[0]);

         return;

      /*
       *  Anything else is a run-time error.
       */

      default :

         abend(SETL_SYSTEM msg_bad_arg,"integer or real or string",1,"abs",
               abend_opnd_str(SETL_SYSTEM argv));

   }
}

/*\
 *  \function{setl2\_even()}
 *
 *  This function is the \verb"even" built-in function.  We just check
 *  the low-order bit and set the target to true or false.
\*/

void setl2_even(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
integer_h_ptr_type integer_hdr;        /* integer header pointer            */

   /* even is only valid for integers */

   switch (argv[0].sp_form) {

      /*
       *  If the source is short, we find the absolute value and check
       *  the low order bit.
       */

      case ft_short :

         if (!(labs(argv[0].sp_val.sp_short_value) & 0x01)) {

            unmark_specifier(target);
            target->sp_form = ft_atom;
            target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

            return;

         }
         else {

            unmark_specifier(target);
            target->sp_form = ft_atom;
            target->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

            return;

         }

      /*
       *  If the source is long we check the low order bit of the low
       *  order cell.
       */

      case ft_long :

         integer_hdr = argv[0].sp_val.sp_long_ptr;

         if (!((integer_hdr->i_head)->i_cell_value & 0x01)) {

            unmark_specifier(target);
            target->sp_form = ft_atom;
            target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

            return;

         }
         else {

            unmark_specifier(target);
            target->sp_form = ft_atom;
            target->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

            return;

         }

      /*
       *  Anything else is a run-time error.
       */

      default :

         abend(SETL_SYSTEM msg_bad_arg,"integer",1,"even",
               abend_opnd_str(SETL_SYSTEM argv));

   }
}

/*\
 *  \function{setl2\_odd()}
 *
 *  This function is the \verb"odd" built-in function.   We just check
 *  the low-order bit and set the target to true or false.
\*/

void setl2_odd(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
integer_h_ptr_type integer_hdr;        /* integer header pointer            */

   /* odd is only valid for integers */

   switch (argv[0].sp_form) {

      /*
       *  If the source is short, we find the absolute value and check
       *  the low order bit.
       */

      case ft_short :

         if (labs(argv[0].sp_val.sp_short_value) & 0x01) {

            unmark_specifier(target);
            target->sp_form = ft_atom;
            target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

            return;

         }
         else {

            unmark_specifier(target);
            target->sp_form = ft_atom;
            target->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

            return;

         }

      /*
       *  If the source is long we check the low order bit of the low
       *  order cell.
       */

      case ft_long :

         integer_hdr = argv[0].sp_val.sp_long_ptr;

         if ((integer_hdr->i_head)->i_cell_value & 0x01) {

            unmark_specifier(target);
            target->sp_form = ft_atom;
            target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

            return;

         }
         else {

            unmark_specifier(target);
            target->sp_form = ft_atom;
            target->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

            return;

         }

      /*
       *  Anything else is a run-time error.
       */

      default :

         abend(SETL_SYSTEM msg_bad_arg,"integer",1,"odd",
               abend_opnd_str(SETL_SYSTEM argv));

   }
}

/*\
 *  \function{setl2\_float()}
 *
 *  This function is the \verb"float" built-in function.
\*/

void setl2_float(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */

   /* float is only valid for integers */

   switch (argv[0].sp_form) {

      /*
       *  If the source is short, we use a C typecast to perform the
       *  conversion.
       */

      case ft_short :

         real_number = (double)(argv[0].sp_val.sp_short_value);

         /* allocate a real */

         unmark_specifier(target);
         i_get_real(real_ptr);
         target->sp_form = ft_real;
         target->sp_val.sp_real_ptr = real_ptr;
         real_ptr->r_use_count = 1;
         real_ptr->r_value = real_number;

         return;

      /*
       *  If the source is long we use a function in the integer package
       *  to perform the conversion.
       */

      case ft_long :

         real_number = long_to_double(SETL_SYSTEM argv);

         /* allocate a real */

         unmark_specifier(target);
         i_get_real(real_ptr);
         target->sp_form = ft_real;
         target->sp_val.sp_real_ptr = real_ptr;
         real_ptr->r_use_count = 1;
         real_ptr->r_value = real_number;

         return;

      /*
       *  Anything else is a run-time error.
       */

      default :

         abend(SETL_SYSTEM msg_bad_arg,"integer",1,"float",
               abend_opnd_str(SETL_SYSTEM argv));

   }
}

/*\
 *  \function{setl2\_atan2()}
 *
 *  This function is the \verb"atan2" built-in function. Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_atan2(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 2 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;
double real_input2;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"atan2",
            abend_opnd_str(SETL_SYSTEM argv));

   }


   if (argv[1].sp_form == ft_real) {
   
   		real_input2 = (argv[1].sp_val.sp_real_ptr)->r_value;

   } else if (argv[1].sp_form == ft_short) {
   		
   		real_input2 = (double)(argv[1].sp_val.sp_short_value);
   	
   } else if (argv[1].sp_form == ft_long) {
   
   		real_input2 = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
   
      abend(SETL_SYSTEM msg_bad_arg,"real",2,"atan2",
            abend_opnd_str(SETL_SYSTEM argv+1));

   }

   /* call the C library function and check for an error */

#ifdef OLD_AND_WRONG_SNYDER_VERSION
   real_number = atan(real_input / real_input2);
#else
   real_number = atan2(real_input,real_input2);
#endif

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_fix()}
 *
 *  This function is the \verb"fix" built-in function.  It is probably
 *  the ugliest, most low-level, and least portable function in the
 *  entire system.  Essentially we have two choices in the implementation
 *  of this function: 1) we could work with doubles and ints directly, in
 *  which case we could preserve portabability but would have problems
 *  with rounding error, or 2) we could work with bits directly, in which
 *  case we have ugly, non-portable code.  We have chosen the second
 *  option.
 *
 *  There is a version of this function for each floating point format we
 *  support.  I'm not going to go into much detail about these functions,
 *  just be certain you know the bit field format for the floating point
 *  format you are examining.  I use a character pointer to examine bit
 *  fields.
 *
 *  The first function picks apart IEEE long format, little-endian.  This
 *  is used on MSDOS machines, and possibly others.
\*/

#if IEEE_LEND

void setl2_fix(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"fix",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 7;

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p--) & 0x7f) << 4;
   exponent |= (*p & 0xf0) >> 4;
   exponent -= ((1 << 10) - 2);

   /* negative exponent => 0.0 */

   if ((!sign && exponent == -((1 << 10) - 2)) || exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value));
   curr_byte = *p++;
   curr_bits_left = 8;
   bits_left = 45;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 5) {

         curr_byte = (*p & 0x0f) | 0x10;
         bits_left = 0;
         curr_bits_left = 5;

      }
      else if (bits_left) {

         curr_byte = *p++;
         curr_bits_left = 8;
         bits_left -= 8;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   carry = 0;

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p++;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p++;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_fix()}
 *
 *  This version of \verb"fix" handles IEEE longs, big-endian.  This is
 *  the most common form of IEEE long.
\*/

#if IEEE_BEND

void setl2_fix(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"fix",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value));

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p++) & 0x7f) << 4;
   exponent |= (*p & 0xf0) >> 4;
   exponent -= ((1 << 10) - 2);

   /* negative exponent => 0.0 */

   if ((!sign && exponent == -((1 << 10) - 2)) || exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 7;
   curr_byte = *p--;
   curr_bits_left = 8;
   bits_left = 45;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 5) {

         curr_byte = (*p & 0x0f) | 0x10;
         bits_left = 0;
         curr_bits_left = 5;

      }
      else if (bits_left) {

         curr_byte = *p--;
         curr_bits_left = 8;
         bits_left -= 8;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   carry = 0;

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p--;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p--;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_fix()}
 *
 *  This version of \verb"fix" handles DEC's G floating point format.
 *  It's quite strange -- a combination of big and little endian.
\*/

#if G_FLOAT

void setl2_fix(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */
int odd;                               /* iteration odd or even             */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"fix",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 1;

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p--) & 0x7f) << 4;
   exponent |= (*p & 0xf0) >> 4;
   exponent -= (1 << 10);

   /* sign and exponent zero => 0.0 */

   if ((!sign && exponent == -(1 << 10)) || exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 6;
   curr_byte = *p++;
   odd = YES;
   curr_bits_left = 8;
   bits_left = 45;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 5) {

         curr_byte = (*p & 0x0f) | 0x10;
         bits_left = 0;
         curr_bits_left = 5;

      }
      else if (bits_left) {

         curr_byte = *p;
         curr_bits_left = 8;
         bits_left -= 8;
         if (odd)
            p -= 3;
         else
            p++;
         odd = !odd;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   carry = 0;

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_fix()}
 *
 *  This version of \verb"fix" handles DEC's D floating point format.
 *  It's quite strange -- a combination of big and little endian.
\*/

#if D_FLOAT

void setl2_fix(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */
int odd;                               /* iteration odd or even             */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"fix",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 1;

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p--) & 0x7f) << 1;
   exponent |= (*p & 0x80) >> 7;
   exponent -= (1 << 7);

   /* sign and exponent zero => 0.0 */

   if ((!sign && exponent == -(1 << 7)) || exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 6;
   curr_byte = *p++;
   odd = YES;
   curr_bits_left = 8;
   bits_left = 48;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 8) {

         curr_byte = (*p & 0x7f) | 0x80;
         bits_left = 0;
         curr_bits_left = 8;

      }
      else if (bits_left) {

         curr_byte = *p;
         curr_bits_left = 8;
         bits_left -= 8;
         if (odd)
            p -= 3;
         else
            p++;
         odd = !odd;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   carry = 0;

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 8) {

            curr_byte = (*p & 0x7f) | 0x80;
            bits_left = 0;
            curr_bits_left = 8;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 8) {

            curr_byte = (*p & 0x7f) | 0x80;
            bits_left = 0;
            curr_bits_left = 8;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_floor()}
 *
 *  This function is the \verb"floor" built-in function.  See the notes
 *  with \verb"setl2_fix", since those comments apply here as well.
 *
 *  This version is for IEEE little-endian.
\*/

#if IEEE_LEND

void setl2_floor(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"floor",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 7;

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p--) & 0x7f) << 4;
   exponent |= (*p & 0xf0) >> 4;
   exponent -= ((1 << 10) - 2);

   /* negative exponent => 0.0 or -1.0, depending on sign */

   if ((!sign && exponent == -((1 << 10) - 2)) || exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0 - sign;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value));
   curr_byte = *p++;
   curr_bits_left = 8;
   bits_left = 45;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 5) {

         curr_byte = (*p & 0x0f) | 0x10;
         bits_left = 0;
         curr_bits_left = 5;

      }
      else if (bits_left) {

         curr_byte = *p++;
         curr_bits_left = 8;
         bits_left -= 8;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   if (sign) {

      if (fraction)
         carry = 1;
      else
         carry = 0;

   }
   else {

      carry = 0;

   }

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p++;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p++;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_floor()}
 *
 *  This version of \verb"floor" handles the IEEE big-endian floating
 *  point format.
\*/

#if IEEE_BEND

void setl2_floor(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"floor",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value));

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p++) & 0x7f) << 4;
   exponent |= (*p & 0xf0) >> 4;
   exponent -= ((1 << 10) - 2);

   if ((!sign && exponent == -((1 << 10) - 2)) || exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0 - sign;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 7;
   curr_byte = *p--;
   curr_bits_left = 8;
   bits_left = 45;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 5) {

         curr_byte = (*p & 0x0f) | 0x10;
         bits_left = 0;
         curr_bits_left = 5;

      }
      else if (bits_left) {

         curr_byte = *p--;
         curr_bits_left = 8;
         bits_left -= 8;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   if (sign) {

      if (fraction)
         carry = 1;
      else
         carry = 0;

   }
   else {

      carry = 0;

   }

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p--;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p--;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_floor()}
 *
 *  This version of \verb"floor" handles DEC's G floating point format.
\*/

#if G_FLOAT

void setl2_floor(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */
int odd;                               /* iteration odd or even             */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"floor",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 1;

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p--) & 0x7f) << 4;
   exponent |= (*p & 0xf0) >> 4;
   exponent -= (1 << 10);

   /* sign and exponent zero => 0.0 */

   if (!sign && exponent == -(1 << 10)) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0;

      return;

   }

   /* negative exponent => 0.0 or -1.0, depending on sign */

   if (exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0 - sign;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 6;
   curr_byte = *p++;
   odd = YES;
   curr_bits_left = 8;
   bits_left = 45;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 5) {

         curr_byte = (*p & 0x0f) | 0x10;
         bits_left = 0;
         curr_bits_left = 5;

      }
      else if (bits_left) {

         curr_byte = *p;
         curr_bits_left = 8;
         bits_left -= 8;
         if (odd)
            p -= 3;
         else
            p++;
         odd = !odd;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   if (sign) {

      if (fraction)
         carry = 1;
      else
         carry = 0;

   }
   else {

      carry = 0;

   }

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_floor()}
 *
 *  This version of \verb"floor" handles DEC's D floating point format.
\*/

#if D_FLOAT

void setl2_floor(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */
int odd;                               /* iteration odd or even             */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"floor",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 1;

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p--) & 0x7f) << 1;
   exponent |= (*p & 0x80) >> 7;
   exponent -= (1 << 7);

   /* sign and exponent zero => 0.0 */

   if (!sign && exponent == -(1 << 7)) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0;

      return;

   }

   /* negative exponent => 0.0 or -1.0, depending on sign */

   if (exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0 - sign;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 6;
   curr_byte = *p++;
   odd = YES;
   curr_bits_left = 8;
   bits_left = 48;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 8) {

         curr_byte = (*p & 0x7f) | 0x80;
         bits_left = 0;
         curr_bits_left = 8;

      }
      else if (bits_left) {

         curr_byte = *p;
         curr_bits_left = 8;
         bits_left -= 8;
         if (odd)
            p -= 3;
         else
            p++;
         odd = !odd;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   if (sign) {

      if (fraction)
         carry = 1;
      else
         carry = 0;

   }
   else {

      carry = 0;

   }

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 8) {

            curr_byte = (*p & 0x7f) | 0x80;
            bits_left = 0;
            curr_bits_left = 8;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 8) {

            curr_byte = (*p & 0x7f) | 0x80;
            bits_left = 0;
            curr_bits_left = 8;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_ceil()}
 *
 *  This function is the \verb"ceil" built-in function.   See the notes
 *  with \verb"setl2_fix", since those comments apply here as well.
 *
 *  This version handles the IEEE little-endian floating point format.
\*/

#if IEEE_LEND

void setl2_ceil(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"ceil",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 7;

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p--) & 0x7f) << 4;
   exponent |= (*p & 0xf0) >> 4;
   exponent -= ((1 << 10) - 2);

   /* zero exponent => 0.0 */

   if (!sign && exponent == -((1 << 10) - 2)) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0;

      return;

   }

   /* negative exponent => 0.0 */

   if (exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 1 - sign;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value));
   curr_byte = *p++;
   curr_bits_left = 8;
   bits_left = 45;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 5) {

         curr_byte = (*p & 0x0f) | 0x10;
         bits_left = 0;
         curr_bits_left = 5;

      }
      else if (bits_left) {

         curr_byte = *p++;
         curr_bits_left = 8;
         bits_left -= 8;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   if (!sign) {

      if (fraction)
         carry = 1;
      else
         carry = 0;

   }
   else {

      carry = 0;

   }

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p++;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p++;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_ceil()}
 *
 *  This version of the \verb"ceil" function handles the IEEE big-endian
 *  format.
\*/

#if IEEE_BEND

void setl2_ceil(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"ceil",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value));

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p++) & 0x7f) << 4;
   exponent |= (*p & 0xf0) >> 4;
   exponent -= ((1 << 10) - 2);

   /* zero exponent => 0.0 */

   if (!sign && exponent == -((1 << 10) - 2)) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0;

      return;

   }

   /* negative exponent => 0.0 */

   if (exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 1 - sign;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 7;
   curr_byte = *p--;
   curr_bits_left = 8;
   bits_left = 45;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (bits_left < exponent) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 5) {

         curr_byte = (*p & 0x0f) | 0x10;
         bits_left = 0;
         curr_bits_left = 5;

      }
      else if (bits_left) {

         curr_byte = *p--;
         curr_bits_left = 8;
         bits_left -= 8;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   if (!sign) {

      if (fraction)
         carry = 1;
      else
         carry = 0;

   }
   else {

      carry = 0;

   }

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p--;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p--;
            curr_bits_left = 8;
            bits_left -= 8;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_ceil()}
 *
 *  This version of the \verb"ceil" function handles DEC's G floating
 *  point format.
\*/

#if G_FLOAT

void setl2_ceil(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */
int odd;                               /* iteration odd or even             */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"ceil",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 1;

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p--) & 0x7f) << 4;
   exponent |= (*p & 0xf0) >> 4;
   exponent -= (1 << 10);

   /* sign and exponent zero => 0.0 */

   if (!sign && exponent == -(1 << 10)) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0;

      return;

   }

   /* negative exponent => 0.0 or 1.0, depending on sign */

   if (exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 1 - sign;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 6;
   curr_byte = *p++;
   odd = YES;
   curr_bits_left = 8;
   bits_left = 45;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 5) {

         curr_byte = (*p & 0x0f) | 0x10;
         bits_left = 0;
         curr_bits_left = 5;

      }
      else if (bits_left) {

         curr_byte = *p;
         curr_bits_left = 8;
         bits_left -= 8;
         if (odd)
            p -= 3;
         else
            p++;
         odd = !odd;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   if (!sign) {

      if (fraction)
         carry = 1;
      else
         carry = 0;

   }
   else {

      carry = 0;

   }

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 5) {

            curr_byte = (*p & 0x0f) | 0x10;
            bits_left = 0;
            curr_bits_left = 5;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_ceil()}
 *
 *  This version of the \verb"ceil" function handles DEC's D floating
 *  point format.
\*/

#if D_FLOAT

void setl2_ceil(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short result                      */
int sign;                              /* YES if the real is negative       */
int exponent;                          /* real exponent (number of bits in  */
                                       /* integer version)                  */
int fraction;                          /* YES if the real has a non-zero    */
                                       /* fractional part                   */
int bits_left;                         /* bits left in mantissa             */
unsigned char curr_byte;               /* current byte                      */
int curr_bits_left;                    /* bits left in current byte         */
int bits_to_get;                       /* bits to be extracted into a cell  */
int shift_dist;                        /* temporary (shift distance)        */
int32 carry;                           /* carry bit                         */
integer_h_ptr_type integer_hdr;        /* header of created integer         */
integer_c_ptr_type integer_cell;       /* cell in created integer           */
int32 short_hi_bits;                   /* high order bits of short          */
unsigned char *p;                      /* temporary looping variable        */
int odd;                               /* iteration odd or even             */

   /* this function is only valid for reals */

   if (argv[0].sp_form != ft_real) {

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"ceil",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set a character pointer to the start of the real value */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 1;

   /* pick out the sign */

   sign = (*p & 0x80) >> 7;

   /* pick out the exponent */

   exponent = (*(p--) & 0x7f) << 1;
   exponent |= (*p & 0x80) >> 7;
   exponent -= (1 << 7);

   /* sign and exponent zero => 0.0 */

   if (!sign && exponent == -(1 << 7)) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 0;

      return;

   }

   /* negative exponent => 0.0 or 1.0, depending on sign */

   if (exponent < 0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 1 - sign;

      return;

   }

   /*
    *  First we skip past the fraction.  We keep track of whether or not
    *  there is a non-zero fraction though.
    */

   p = ((unsigned char *)&(argv[0].sp_val.sp_real_ptr->r_value)) + 6;
   curr_byte = *p++;
   odd = YES;
   curr_bits_left = 8;
   bits_left = 48;
   fraction = NO;

   while (bits_left + curr_bits_left > exponent) {

      /* special case for last byte */

      if (!bits_left || (bits_left < exponent)) {

         shift_dist = bits_left + curr_bits_left - exponent;
         fraction = fraction || (curr_byte & ((1 << shift_dist) - 1));
         curr_byte >>= shift_dist;
         curr_bits_left -= shift_dist;

         break;

      }

      /* we check the entire current byte for a fraction */

      fraction = fraction || curr_byte;

      /* set up for the next byte */

      if (bits_left == 8) {

         curr_byte = (*p & 0x7f) | 0x80;
         bits_left = 0;
         curr_bits_left = 8;

      }
      else if (bits_left) {

         curr_byte = *p;
         curr_bits_left = 8;
         bits_left -= 8;
         if (odd)
            p -= 3;
         else
            p++;
         odd = !odd;

      }
      else {

         curr_byte = 0;
         curr_bits_left = 8;

      }
   }

   /*
    *  Now we have some bits in curr_byte, perhaps more pointed to by p.
    */

   if (!sign) {

      if (fraction)
         carry = 1;
      else
         carry = 0;

   }
   else {

      carry = 0;

   }

   /* if the result might fit in a short ... */

   if (exponent <= INT_CELL_WIDTH + 1) {

      shift_dist = 0;
      short_value = 0;
      bits_to_get = exponent;

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            short_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         short_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 8) {

            curr_byte = (*p & 0x7f) | 0x80;
            bits_left = 0;
            curr_bits_left = 8;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }
      }

      /* add in the carry */

      short_value += carry;

      /* set the sign */

      if (sign)
         short_value = -short_value;

      /* check whether the result remains short */

      if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
          short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_value;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_value);

      return;

   }

   /*
    *  The exponent is greater than the width of an integer cell.  We are
    *  certain we must build a long integer.
    */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_cell_count = 1;
   integer_hdr->i_is_negative = sign;

   get_integer_cell(integer_cell)
   integer_cell->i_next = integer_cell->i_prev = NULL;
   integer_hdr->i_head = integer_hdr->i_tail = integer_cell;
   integer_cell->i_cell_value = 0;

   /* if we lost significance, abend */

   if (exponent > 53)
      abend(SETL_SYSTEM "Loss of significance in integer conversion:\nReal => %s",
            abend_opnd_str(SETL_SYSTEM argv));

   /* pick out each cell */

   if (exponent > 53) {

      bits_to_get = INT_CELL_WIDTH - (exponent - 53);
      shift_dist = exponent - 53;
      exponent -= INT_CELL_WIDTH;

   }
   else {

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

   }

   for (;;) {

      while (bits_to_get) {

         /* special case for last byte */

         if (bits_to_get < curr_bits_left) {

            integer_cell->i_cell_value |=
               (curr_byte & ((1 << bits_to_get) - 1)) << shift_dist;
            curr_byte >>= bits_to_get;
            curr_bits_left -= bits_to_get;

            break;

         }

         integer_cell->i_cell_value |= (curr_byte << shift_dist);
         shift_dist += curr_bits_left;
         bits_to_get -= curr_bits_left;

         /* set up for the next byte */

         if (bits_left == 8) {

            curr_byte = (*p & 0x7f) | 0x80;
            bits_left = 0;
            curr_bits_left = 8;

         }
         else if (bits_left) {

            curr_byte = *p;
            curr_bits_left = 8;
            bits_left -= 8;
            if (odd)
               p -= 3;
            else
               p++;
            odd = !odd;

         }
         else {

            curr_byte = 0;
            curr_bits_left = 8;

         }

         /* add in the carry */

         integer_cell->i_cell_value += carry;
         carry = integer_cell->i_cell_value >> INT_CELL_WIDTH;
         integer_cell->i_cell_value &= MAX_INT_CELL;

      }

      bits_to_get = min(exponent,INT_CELL_WIDTH);
      exponent -= bits_to_get;
      shift_dist = 0;

      if (!bits_to_get)
         break;

      /* add another cell */

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = 0;

   }

   /* we may have a carry left */

   if (carry) {

      get_integer_cell(integer_cell);
      (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_cell->i_next = NULL;
      integer_hdr->i_tail = integer_cell;
      integer_hdr->i_cell_count++;
      integer_cell->i_cell_value = carry;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = integer_hdr;

   return;

}

#endif

/*\
 *  \function{setl2\_exp()}
 *
 *  This function is the \verb"exp" built-in function.  Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_exp(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
  
      abend(SETL_SYSTEM msg_bad_arg,"real",1,"exp",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* call the C library function and check for an error */

   real_number = exp(real_input);

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_log()}
 *
 *  This function is the \verb"log" built-in function.  Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_log(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
   

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"log",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* call the C library function and check for an error */

   real_number = log(real_input);

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_cos()}
 *
 *  This function is the \verb"cos" built-in function.  Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_cos(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
 

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"cos",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* call the C library function and check for an error */

   real_number = cos(real_input);

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_sin()}
 *
 *  This function is the \verb"sin" built-in function.  Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_sin(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
 

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"sin",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* call the C library function and check for an error */

   real_number = sin(real_input);

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_tan()}
 *
 *  This function is the \verb"tan" built-in function.  Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_tan(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
 
      abend(SETL_SYSTEM msg_bad_arg,"real",1,"tan",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* call the C library function and check for an error */

   real_number = tan(real_input);

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_acos()}
 *
 *  This function is the \verb"acos" built-in function. Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_acos(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
 
      abend(SETL_SYSTEM msg_bad_arg,"real",1,"acos",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* call the C library function and check for an error */

   real_number = acos(real_input);

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_asin()}
 *
 *  This function is the \verb"asin" built-in function. Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_asin(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
  
      abend(SETL_SYSTEM msg_bad_arg,"real",1,"asin",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* call the C library function and check for an error */

   real_number = asin(real_input);

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_atan()}
 *
 *  This function is the \verb"atan" built-in function. Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_atan(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
   
      abend(SETL_SYSTEM msg_bad_arg,"real",1,"atan",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* call the C library function and check for an error */

   real_number = atan(real_input);

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_tanh()}
 *
 *  This function is the \verb"tanh" built-in function. Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_tanh(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
  

      abend(SETL_SYSTEM msg_bad_arg,"real",1,"tanh",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* call the C library function and check for an error */

   real_number = tanh(real_input);

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_sqrt()}
 *
 *  This function is the \verb"sqrt" built-in function. Like most of the
 *  built-in math functions we simply call the C library function.
\*/

void setl2_sqrt(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
double real_input;

   if (argv[0].sp_form == ft_real) {
   
   		real_input = (argv[0].sp_val.sp_real_ptr)->r_value;

   } else if (argv[0].sp_form == ft_short) {
   		
   		real_input = (double)(argv[0].sp_val.sp_short_value);
   	
   } else if (argv[0].sp_form == ft_long) {
   
   		real_input = (double) (long_to_double(SETL_SYSTEM argv));
   
   } else {
   
      abend(SETL_SYSTEM msg_bad_arg,"real",1,"sqrt",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* call the C library function and check for an error */

   real_number = sqrt(real_input);

#if INFNAN

   if (NANorINF(real_number))
      abend(SETL_SYSTEM "Floating point error -- Not a number");

#else

   if (errno == EDOM)
      abend(SETL_SYSTEM msg_domain_error);
   if (errno == ERANGE)
      abend(SETL_SYSTEM msg_range_error);

#endif

   /* allocate a real */

   unmark_specifier(target);
   i_get_real(real_ptr);
   target->sp_form = ft_real;
   target->sp_val.sp_real_ptr = real_ptr;
   real_ptr->r_use_count = 1;
   real_ptr->r_value = real_number;

   return;

}

/*\
 *  \function{setl2\_sign()}
 *
 *  This function is the \verb"sign" built-in function.  We just examine
 *  the sign of the argument.
\*/

void setl2_sign(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
int32 short_value;                     /* short integer value               */

   switch (argv[0].sp_form) {

      case ft_short :

         if (argv[0].sp_val.sp_short_value < 0)
            short_value = -1;
         else if (argv[0].sp_val.sp_short_value > 0)
            short_value = 1;
         else
            short_value = 0;

         break;

      case ft_long :

         if (argv[0].sp_val.sp_long_ptr->i_is_negative)
            short_value = -1;
         else
            short_value = 1;

         break;

      case ft_real :

         if ((argv[0].sp_val.sp_real_ptr)->r_value < 0)
            short_value = -1;
         else if ((argv[0].sp_val.sp_real_ptr)->r_value > 0)
            short_value = 1;
         else
            short_value = 0;

         break;

      default :

         abend(SETL_SYSTEM msg_bad_arg,"real",1,"sign",
               abend_opnd_str(SETL_SYSTEM argv));

   }

   /* set the target */

   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value = short_value;

   return;

}

