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
 *  \packagespec{Specifiers}
\*/

#ifndef SPECS_LOADED

/* standard C header files */

#if WATCOM
#include <dos.h>                       /* DOS use macros                    */
#endif

/* opaque object structure */
 
struct opaque_item {
   int32 use_count;                    /* usage count                       */
   int32 type;                         /* opaque type                       */
}; 

typedef struct opaque_item *opaque_item_ptr_type;

/* specifier structure */

struct specifier_item {
   unsigned sp_form;                   /* form code                         */
   union {
      int32 sp_atom_num;               /* unique atom identifier            */
      int32 sp_short_value;            /* value of short item               */
      struct file_item *sp_file_ptr;   /* file node pointer                 */
      struct instruction_item *sp_label_ptr;
                                       /* label value code pointer          */
      struct opaque_item *sp_opaque_ptr;
                                       /* opaque item                       */      
   		struct proc_item *sp_proc_ptr;   /* procedure                         */
      struct integer_h_item *sp_long_ptr;
                                       /* header of integer                 */
      struct i_real_item *sp_real_ptr; /* real number                       */
      struct string_h_item *sp_string_ptr;
                                       /* header of string                  */
      struct set_h_item *sp_set_ptr;   /* root of set header                */
      struct map_h_item *sp_map_ptr;   /* root of map header                */
      struct tuple_h_item *sp_tuple_ptr;
                                       /* root of tuple header              */
      struct iter_item *sp_iter_ptr;   /* iterator                          */
      struct object_h_item *sp_object_ptr;
                                       /* object (user-defined)             */
      struct mailbox_h_item *sp_mailbox_ptr;
                                       /* mailbox header                    */
      void *sp_biggest;                /* dummy -- it must be as large as   */
                                       /* the largest of the above          */
   } sp_val;
};

typedef struct specifier_item specifier;
                                       /* specifier item                    */

/*\
 *  \function{mark\_specifier()}
 *
 *  This function bumps the use count of a specifier (long items only).
 *  It assumes the use count is an integer, and is the first field of the
 *  header of a long item.
\*/

#define mark_specifier(s) { \
   if ((s)->sp_form >= ft_opaque) \
      (*((int32 *)((s)->sp_val.sp_biggest)))++; \
}

/*\
 *  \function{unmark\_specifier()}
 *
 *  This function decrements the use count of a specifier (long items
 *  only), and releases the item if the use count drops to zero.  It
 *  assumes the use count is an integer, and is the first field of the
 *  header of a long item.
\*/

#define unmark_specifier(s) { \
   if ((s)->sp_form >= ft_opaque && \
          !(--(*((int32 *)((s)->sp_val.sp_biggest))))) \
      free_specifier(SETL_SYSTEM s); \
}

#if !SHORT_MACROS

#include "specmacs.h"

#else

#define spec_equal(t,l,r) { \
   t = spec_equal_mac(SETL_SYSTEM l,r); \
}

#define spec_hash_code(t,s) {\
   t = spec_hash_code_mac(s); \
}

int spec_equal_mac(SETL_SYSTEM_PROTO specifier *, specifier *);
                                       /* specifier equality test           */
int32 spec_hash_code_mac(specifier *); /* hash code calculation             */
#endif

/* public function declarations */

struct specifier_item *get_specifiers(SETL_SYSTEM_PROTO int32);
                                       /* allocate a block of specifiers    */
void free_specifier(SETL_SYSTEM_PROTO struct specifier_item *);
                                       /* release memory used by specifier  */
int spec_equal_test(SETL_SYSTEM_PROTO specifier *, specifier *);
                                       /* compare two specifiers            */
int32 spec_hash_code_calc(specifier *);
                                       /* calculate hash code               */

#define SPECS_LOADED 1
#endif


