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
 *  \package{Built-In String Procedures}
 *
 *  In this package we have built-in procedures which are
 *  string-oriented.  Most of these are for string scanning, and we hope
 *  to someday move those into a library.
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "mcode.h"                     /* method codes                      */
#include "builtins.h"                  /* built-in symbols                  */
#include "unittab.h"                   /* unit table                        */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_reals.h"                   /* real numbers                      */
#include "x_strngs.h"                  /* strings                           */
#include "sets.h"                      /* sets                              */
#include "maps.h"                      /* maps                              */
#include "tuples.h"                    /* tuples                            */
#include "objects.h"                   /* objects                           */
#include "process.h"                   /* objects                           */
#include "mailbox.h"                   /* objects                           */
#include "slots.h"                     /* procedures                        */

/*
 *  Some compilers, notably Lattice, can not handle a very long text
 *  segment.  This stuff is used to break up this file, so we can run
 *  through it several times.
 */

#ifdef SHORT_TEXT

#define PRTYPE                         /* prototype class                   */

#ifdef ROOT
#define PCLASS                         /* data storage class                */
#else
#define PCLASS extern                  /* data storage class                */
#endif

#ifndef ROOT
#define ROOT 0
#endif

#ifndef PARTA
#define PARTA 0
#endif

#ifndef PARTB
#define PARTB 0
#endif

#ifndef PARTC
#define PARTC 0
#endif

#ifndef PARTD
#define PARTD 0
#endif

#ifndef PARTF
#define PARTF 0
#endif

#ifndef PARTG
#define PARTG 0
#endif

#ifndef PARTH
#define PARTH 0
#endif

#else
#define PCLASS static                  /* data storage class                */
#define PRTYPE static                  /* prototype class                   */
#endif

/* package global variables */

PCLASS string_h_ptr_type str_curr_hdr; /* str return string                 */
PCLASS string_c_ptr_type str_curr_cell;
                                       /* current cell in above             */
PCLASS char *str_char_ptr, *str_char_end;
                                       /* character pointers in above       */
PCLASS char character_set[256];        /* character set (bit vector sortof) */

/* forward declarations */

PRTYPE void str_cat_spec(SETL_SYSTEM_PROTO specifier *);
                                       /* concatenate a specifier on str's  */
                                       /* return value                      */
PRTYPE void str_cat_string(SETL_SYSTEM_PROTO char *); 
                                       /* concatenate a string on str's     */
                                       /* return value                      */

#if !SHORT_TEXT | ROOT

/*\
 *  \function{setl2\_char()}
 *
 *  This function is the \verb"char" built-in function.  We just form a
 *  character from an integer.
\*/

void setl2_char(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* string header pointer             */
string_c_ptr_type string_cell;         /* string cell pointer               */

   /* char is only valid for integers */

   if (argv[0].sp_form != ft_short ||
       argv[0].sp_val.sp_short_value < 0 ||
       argv[0].sp_val.sp_short_value > 256)
      abend(SETL_SYSTEM msg_bad_arg,"integer",1,"char",
            abend_opnd_str(SETL_SYSTEM argv));

   /* create a one-character string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 1;
   get_string_cell(string_cell);
   string_cell->s_next = string_cell->s_prev = NULL;
   string_hdr->s_head = string_hdr->s_tail = string_cell;
   string_cell->s_cell_value[0] = (char)(argv[0].sp_val.sp_short_value);

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;

}

/*\
 *  \function{setl2\_str()}
 *
 *  This function is the \verb"str" built-in function.  We initialize a
 *  string, then call a recursive function which generates the string
 *  contents.
\*/

void setl2_str(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type save_str_curr_hdr;  /* str return string                 */
string_c_ptr_type save_str_curr_cell; /* current cell in above             */
char *save_str_char_ptr, *save_str_char_end;
                                       /* character pointers in above       */

   /* save string variables in case we're called recursively */

   save_str_curr_hdr = str_curr_hdr;
   save_str_curr_cell = str_curr_cell;
   save_str_char_ptr = str_char_ptr;
   save_str_char_end = str_char_end;

   /* initialize the return string */

   get_string_header(str_curr_hdr);
   str_curr_hdr->s_use_count = 1;
   str_curr_hdr->s_hash_code = -1;
   str_curr_hdr->s_length = 0;
   str_curr_hdr->s_head = str_curr_hdr->s_tail = str_curr_cell = NULL;
   str_char_ptr = str_char_end = NULL;

   /* call a recursive function to make the string */

   str_cat_spec(SETL_SYSTEM argv);

   /* set the return value and return */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = str_curr_hdr;

   /* restore string variables */

   str_curr_hdr = save_str_curr_hdr;
   str_curr_cell = save_str_curr_cell;
   str_char_ptr = save_str_char_ptr;
   str_char_end = save_str_char_end;

   return;

}

/*\
 *  \function{str\_cat\_spec()}
 *
 *  This function appends a single specifier on the \verb"str" return
 *  value.  It calls itself recursively for sets and tuples.
\*/

PRTYPE void str_cat_spec(
   SETL_SYSTEM_PROTO
   specifier *spec)                    /* specifier to be printed           */

{
char tmpstring[100];                   /* work string (for sprintf)         */

   switch (spec->sp_form) {

/*\
 *  \case{unprintable types}
 *
 *  We have a few types which we hope will not be printed, since we can
 *  not print anything meaningful for them.  We do allow these types to
 *  be printed, but just print something to let the operator know the
 *  type of the thing he printed.
\*/

case ft_omega :

{

   str_cat_string(SETL_SYSTEM "<om>");

   return;

}

case ft_atom :

{

   if (spec->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num) {
      strcpy(tmpstring,"TRUE");
   }
   else if (spec->sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num) {
      strcpy(tmpstring,"FALSE");
   }
   else {
      sprintf(tmpstring,"<atom %ld>",spec->sp_val.sp_atom_num);
   }

   str_cat_string(SETL_SYSTEM tmpstring);

   return;

}

case ft_opaque :

{

#if WATCOM

   sprintf(tmpstring,"<opaque %p>",spec->sp_val.sp_opaque_ptr);
   str_cat_string(SETL_SYSTEM tmpstring);

#else

   sprintf(tmpstring,"<opaque %ld>",(int32)(spec->sp_val.sp_opaque_ptr));
   str_cat_string(SETL_SYSTEM tmpstring);

#endif

   return;

}

case ft_label :

{

#if WATCOM

   sprintf(tmpstring,"<label %p>",spec->sp_val.sp_label_ptr);
   str_cat_string(SETL_SYSTEM tmpstring);

#else

   sprintf(tmpstring,"<label %ld>",(int32)(spec->sp_val.sp_label_ptr));
   str_cat_string(SETL_SYSTEM tmpstring);

#endif

   return;

}

case ft_file :

{

#if WATCOM

   sprintf(tmpstring,"<file %p>",spec->sp_val.sp_file_ptr);
   str_cat_string(SETL_SYSTEM tmpstring);

#else

   sprintf(tmpstring,"<file %ld>",(int32)(spec->sp_val.sp_file_ptr));
   str_cat_string(SETL_SYSTEM tmpstring);

#endif

   return;

}

case ft_proc :

{

#if WATCOM

   sprintf(tmpstring,"<procedure %p>",spec->sp_val.sp_proc_ptr);
   str_cat_string(SETL_SYSTEM tmpstring);

#else

   sprintf(tmpstring,"<procedure %ld>",(int32)(spec->sp_val.sp_proc_ptr));
   str_cat_string(SETL_SYSTEM tmpstring);

#endif

   return;

}

case ft_mailbox :

{
mailbox_c_ptr_type cell_ptr;           /* mailbox cell                      */
int first_element;                     /* first entry switch                */

#if WATCOM

   sprintf(tmpstring,"<mailbox %p",spec->sp_val.sp_mailbox_ptr);
   str_cat_string(SETL_SYSTEM tmpstring);

#else

   sprintf(tmpstring,"<mailbox %ld",(int32)(spec->sp_val.sp_mailbox_ptr));
   str_cat_string(SETL_SYSTEM tmpstring);

#endif

   first_element = YES;
   for (cell_ptr = spec->sp_val.sp_mailbox_ptr->mb_head;
        cell_ptr != NULL;
        cell_ptr = cell_ptr->mb_next) {

      if (!first_element)  
         str_cat_string(SETL_SYSTEM ",");
      else
         first_element = NO;

      str_cat_string(SETL_SYSTEM " ");
      str_cat_spec(SETL_SYSTEM &(cell_ptr->mb_spec));

   }

   str_cat_string(SETL_SYSTEM ">");

   return;

}

case ft_iter :

{

#if WATCOM

   sprintf(tmpstring,"<iterator %p>",spec->sp_val.sp_iter_ptr);
   str_cat_string(SETL_SYSTEM tmpstring);

#else

   sprintf(tmpstring,"<iterator %ld>",(int32)(spec->sp_val.sp_iter_ptr));
   str_cat_string(SETL_SYSTEM tmpstring);

#endif

   return;

}

/*\
 *  \case{integers}
 *
 *  We have two kinds of integers: short and long.  Short integers
 *  we can normally handle quite easily, but longs are more work.
\*/

case ft_short :

{

   sprintf(tmpstring,"%ld",spec->sp_val.sp_short_value);
   str_cat_string(SETL_SYSTEM tmpstring);
   return;

}

case ft_long :

{
char *p;                               /* printable integer string          */

   p = integer_string(SETL_SYSTEM spec,10);
   str_cat_string(SETL_SYSTEM p);
   free(p);

   return;

}

/*\
 *  \case{real numbers}
 *
 *  We depend on the C library to do most of the work in printing a real
 *  number.
\*/

case ft_real :

{

   sprintf(tmpstring,"%#.11g",(spec->sp_val.sp_real_ptr)->r_value);
   str_cat_string(SETL_SYSTEM tmpstring);

   return;

}

/*\
 *  \case{strings}
 *
 *  Strings are complex structures, because we allow infinite length.  We
 *  have to print each cell individually, and translate nulls to blanks.
\*/

case ft_string :

{
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
int32 chars_to_print;                  /* characters left in string         */
char cell_string[STR_CELL_WIDTH + 1];  /* printable cell string             */
char *s, *t;                           /* temporary looping variables       */

   /* loop over the cells ... */

   string_hdr = spec->sp_val.sp_string_ptr;
   chars_to_print = string_hdr->s_length;
   for (string_cell = string_hdr->s_head;
        chars_to_print && string_cell != NULL;
        string_cell = string_cell->s_next) {

      /* translate nulls to spaces */

      for (s = string_cell->s_cell_value, t = cell_string;
           s < string_cell->s_cell_value + STR_CELL_WIDTH;
           s++, t++) {

         if (*s == '\0')
            *t = ' ';
         else
            *t = *s;

      }

      /* print the cell (or as much as necessary) */

      if (chars_to_print < STR_CELL_WIDTH) {

         cell_string[chars_to_print] = '\0';
         chars_to_print = 0;

      }
      else {

         cell_string[STR_CELL_WIDTH] = '\0';
         chars_to_print -= STR_CELL_WIDTH;

      }

      str_cat_string(SETL_SYSTEM cell_string);

   }

   return;

}

/*\
 *  \case{sets}
 *
 *  We loop over the elements of a set printing each.  When we encounter
 *  a string we print enclosing quotes.
\*/

case ft_set :

{
int first_element;                     /* YES for the first element         */
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */

   /* print the opening brace, and start looping over the set */

   str_cat_string(SETL_SYSTEM "{");
   first_element = YES;

   source_root = spec->sp_val.sp_set_ptr;
   source_work_hdr = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_cell = NULL;
   source_index = 0;

   /* loop over the elements of source */

   for (;;) {

      /* find the next element in the set */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, break */

         if (source_cell != NULL) {

            source_element = &(source_cell->s_spec);
            source_cell = source_cell->s_next;
            break;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < SET_HASH_SIZE) {

            source_cell = source_work_hdr->s_child[source_index].s_cell;
            source_index++;
            continue;

         }

         /* move up if we're at the end of a node */

         if (source_index >= SET_HASH_SIZE) {

            /* there are no more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->s_ntype.s_intern.s_child_index + 1;
            source_work_hdr =
               source_work_hdr->s_ntype.s_intern.s_parent;
            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->s_child[source_index].s_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->s_child[source_index].s_header;
         source_index = 0;
         source_height--;

      }

      /* if we've exhausted the set break again */

      if (source_element == NULL)
         break;

      /*
       *  At this point we have an element in source_element which must
       *  be printed.
       */

      /* print a comma after the previous element */

      if (first_element)
         first_element = NO;
      else
         str_cat_string(SETL_SYSTEM ", ");

      /* print enclosing quotes around strings */

      if (source_element->sp_form == ft_string) {

         str_cat_string(SETL_SYSTEM "\"");
         str_cat_spec(SETL_SYSTEM source_element);
         str_cat_string(SETL_SYSTEM "\"");

      }

      /* otherwise, just print the element */

      else {

         str_cat_spec(SETL_SYSTEM source_element);

      }
   }

   /* that's it */

   str_cat_string(SETL_SYSTEM "}");

   return;

}

/*\
 *  \case{maps}
 *
 *  We loop over the elements of a map printing each pair.  When we
 *  encounter a string we print enclosing quotes.
\*/

case ft_map :

{
int first_element;                     /* YES for the first element         */
map_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type valset_root, valset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type valset_cell;            /* current cell pointer              */
int valset_height, valset_index;       /* current height and index          */
specifier *domain_element, *range_element;
                                       /* pair from map                     */

   /* print the opening brace, and start looping over the map */

   str_cat_string(SETL_SYSTEM "{");
   first_element = YES;

   source_root = spec->sp_val.sp_map_ptr;
   source_work_hdr = source_root;
   source_height = source_root->m_ntype.m_root.m_height;
   source_cell = NULL;
   source_index = 0;
   valset_root = NULL;

   /* loop over the elements of source */

   for (;;) {

      /* find the next cell in the map */

      while (source_cell == NULL) {

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < MAP_HASH_SIZE) {

            source_cell = source_work_hdr->m_child[source_index].m_cell;
            source_index++;
            continue;

         }


         /* move up if we're at the end of a node */

         if (source_index >= MAP_HASH_SIZE) {

            /* there are no more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->m_ntype.m_intern.m_child_index + 1;
            source_work_hdr =
               source_work_hdr->m_ntype.m_intern.m_parent;
            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->m_child[source_index].m_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->m_child[source_index].m_header;
         source_index = 0;
         source_height--;

      }

      /* if there are no more cells, break */

      if (source_cell == NULL)
         break;

      /* if the cell is not multi-value, use the pair directly */

      if (!(source_cell->m_is_multi_val)) {

         domain_element = &(source_cell->m_domain_spec);
         range_element = &(source_cell->m_range_spec);
         source_cell = source_cell->m_next;

      }

      /* otherwise find the next element in the value set */

      else {

         domain_element = &(source_cell->m_domain_spec);
         if (valset_root == NULL) {

            valset_root = source_cell->m_range_spec.sp_val.sp_set_ptr;
            valset_work_hdr = valset_root;
            valset_height = valset_root->s_ntype.s_root.s_height;
            valset_cell = NULL;
            valset_index = 0;

         }

         range_element = NULL;
         for (;;) {

            /* if we have an element already, break */

            if (valset_cell != NULL) {

               range_element = &(valset_cell->s_spec);
               valset_cell = valset_cell->s_next;
               break;

            }

            /* start on the next clash list, if we're at a leaf */

            if (!valset_height && valset_index < SET_HASH_SIZE) {

               valset_cell = valset_work_hdr->s_child[valset_index].s_cell;
               valset_index++;
               continue;

            }

            /* the current header node is exhausted -- find the next one */

            if (valset_index >= SET_HASH_SIZE) {

               /* there are no more elements, so break */

               if (valset_work_hdr == valset_root)
                  break;

               /* otherwise move up */

               valset_height++;
               valset_index =
                  valset_work_hdr->s_ntype.s_intern.s_child_index + 1;
               valset_work_hdr =
                  valset_work_hdr->s_ntype.s_intern.s_parent;
               continue;

            }

            /* skip over null nodes */

            if (valset_work_hdr->s_child[valset_index].s_header == NULL) {

               valset_index++;
               continue;

            }

            /* otherwise drop down a level */

            valset_work_hdr =
               valset_work_hdr->s_child[valset_index].s_header;
            valset_index = 0;
            valset_height--;

         }

         if (range_element == NULL) {

            source_cell = source_cell->m_next;
            valset_root = NULL;
            continue;

         }
      }

      /*
       *  At this point we have a pair from the map which we would like to
       *  print.
       */

      /* print a comma after the previous element */

      if (first_element)
         first_element = NO;
      else
         str_cat_string(SETL_SYSTEM ", ");

      /* print the domain element */

       str_cat_string(SETL_SYSTEM "[");

      if (domain_element->sp_form == ft_string) {

         str_cat_string(SETL_SYSTEM "\"");
         str_cat_spec(SETL_SYSTEM domain_element);
         str_cat_string(SETL_SYSTEM "\"");

      }

      /* otherwise, just print the element */

      else {

         str_cat_spec(SETL_SYSTEM domain_element);

      }

      /* print the range element */

       str_cat_string(SETL_SYSTEM ", ");

      if (range_element->sp_form == ft_string) {

         str_cat_string(SETL_SYSTEM "\"");
         str_cat_spec(SETL_SYSTEM range_element);
         str_cat_string(SETL_SYSTEM "\"");

      }

      /* otherwise, just print the element */

      else {

         str_cat_spec(SETL_SYSTEM range_element);

      }

       str_cat_string(SETL_SYSTEM "]");

   }

   /* that's it */

   str_cat_string(SETL_SYSTEM "}");

   return;

}

/*\
 *  \case{tuples}
 *
 *  We loop over the elements of a tuple printing each. When we encounter
 *  a string we print enclosing quotes.
\*/

case ft_tuple :

{
int32 printed_number;                  /* number of elements printed        */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */

   /* print the opening bracket, and start looping over the tuple */

   str_cat_string(SETL_SYSTEM "[");
   printed_number = 0;

   source_root = spec->sp_val.sp_tuple_ptr;
   source_work_hdr = source_root;
   source_number = -1;
   source_height = source_root->t_ntype.t_root.t_height;
   source_index = 0;

   /* loop over the elements of source */

   while (source_number < source_root->t_ntype.t_root.t_length) {

      /* find the next element in the tuple */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, return it */

         if (!source_height && source_index < TUP_HEADER_SIZE) {

            if (source_work_hdr->t_child[source_index].t_cell == NULL) {

               source_number++;
               source_index++;

               continue;

            }

            source_cell = source_work_hdr->t_child[source_index].t_cell;
            source_element = &(source_cell->t_spec);

            source_number++;
            source_index++;

            break;

         }

         /* move up if we're at the end of a node */

         if (source_index >= TUP_HEADER_SIZE) {

            /* break if we've exhausted the source */

            if (source_work_hdr == source_root)
               break;

            source_height++;
            source_index =
               source_work_hdr->t_ntype.t_intern.t_child_index + 1;
            source_work_hdr =
               source_work_hdr->t_ntype.t_intern.t_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->t_child[source_index].t_header == NULL) {

            source_number += 1L << (source_height * TUP_SHIFT_DIST);
            source_index++;

            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      if (source_element == NULL)
         break;

      /*
       *  At this point we have an element in source_element which must
       *  be printed.  We might have to print a bunch of OM's first
       *  though.
       */

      /* print a comma after the previous element */

      if (printed_number != 0)
         str_cat_string(SETL_SYSTEM ", ");

      while (printed_number++ < source_number) {

         str_cat_string(SETL_SYSTEM "<om>, ");

      }

      /* print enclosing quotes around strings */

      if (source_element->sp_form == ft_string) {

         str_cat_string(SETL_SYSTEM "\"");
         str_cat_spec(SETL_SYSTEM source_element);
         str_cat_string(SETL_SYSTEM "\"");

      }

      /* otherwise, just print the element */

      else {

         str_cat_spec(SETL_SYSTEM source_element);

      }
   }

   /* that's it */

   str_cat_string(SETL_SYSTEM "]");

   return;

}

/*\
 *  \case{objects}
 *
 *  We print objects by calling the programmer's method, if he gave us one,
 *  and otherwise printing each instance variable.
\*/

case ft_object :
case ft_process :

{
unittab_ptr_type class_ptr;            /* class pointer                     */
object_h_ptr_type object_root, object_work_hdr;
                                       /* object header pointers            */
object_c_ptr_type object_cell;         /* object cell pointer               */
struct slot_info_item *slot_info;      /* slot information record           */
int target_index, target_height;       /* used to descend header trees      */
int32 target_number;                   /* slot number                       */
specifier spare;                       /* spare for user-defined method     */

   /* pick out the object root and class */

   object_root = spec->sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* get information about the str slot in the object */

   slot_info = class_ptr->ut_slot_info + m_str;

   /* if we find a method, execute it */

   if (slot_info->si_in_class) {

      /* call the procedure */

      spare.sp_form = ft_omega;
      call_procedure(SETL_SYSTEM &spare,
                     slot_info->si_spec,
                     spec,
                     0L,YES,YES,0);

      /* concatenate the new string */

      str_cat_spec(SETL_SYSTEM &spare);
      unmark_specifier(&spare);

      return;

   }

   /*
    *  At this point we know that the programmer didn't supply a method
    *  for us, so we use our own.
    */

   object_work_hdr = object_root;
   target_height = class_ptr->ut_obj_height;

   /* print the class type */

   str_cat_string(SETL_SYSTEM "<");
   str_cat_string(SETL_SYSTEM class_ptr->ut_name);
   str_cat_string(SETL_SYSTEM ":");

   /* loop over the instance variables */

   for (slot_info = class_ptr->ut_first_var, target_number = 0;
        slot_info != NULL;
        slot_info = slot_info->si_next_var, target_number++) {

      /* drop down to a leaf */

      while (target_height) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * OBJ_SHIFT_DIST)) &
                           OBJ_SHIFT_MASK;

         /* we'll always have all internal nodes in this situation */

         object_work_hdr =
            object_work_hdr->o_child[target_index].o_header;
         target_height--;

      }

      /*
       *  At this point, object_work_hdr points to the lowest level
       *  header record.  Concatenate the instance variable.
       */

      str_cat_string(SETL_SYSTEM " ");
      str_cat_string(SETL_SYSTEM (slot_info->si_slot_ptr)->sl_name);
      str_cat_string(SETL_SYSTEM " => ");
      target_index = target_number & OBJ_SHIFT_MASK;
      object_cell = object_work_hdr->o_child[target_index].o_cell;
      str_cat_spec(SETL_SYSTEM &(object_cell->o_spec));

      /*
       *  We move back up the header tree at this point, if it is
       *  necessary.
       */

      target_index++;
      while (target_index >= OBJ_HEADER_SIZE) {

         target_height++;
         target_index =
            object_work_hdr->o_ntype.o_intern.o_child_index + 1;
         object_work_hdr = object_work_hdr->o_ntype.o_intern.o_parent;

      }
   }

   str_cat_string(SETL_SYSTEM ">");

   return;

}

/* back to normal indentation */

   }

   return;

}

/*\
 *  \function{str\_cat\_string()}
 *
 *  This function concatenates a C character string on the \verb"str"
 *  return value.
\*/

PRTYPE void str_cat_string(
   SETL_SYSTEM_PROTO
   char *charstring)                   /* C string to be concatenated       */

{
char *p;                               /* temporary looping variable        */

   p = charstring;
   while (*p) {

      if (str_char_ptr == str_char_end) {

         get_string_cell(str_curr_cell);
         if (str_curr_hdr->s_tail != NULL)
            (str_curr_hdr->s_tail)->s_next = str_curr_cell;
         str_curr_cell->s_prev = str_curr_hdr->s_tail;
         str_curr_cell->s_next = NULL;
         str_curr_hdr->s_tail = str_curr_cell;
         if (str_curr_hdr->s_head == NULL)
            str_curr_hdr->s_head = str_curr_cell;
         str_char_ptr = str_curr_cell->s_cell_value;
         str_char_end = str_char_ptr + STR_CELL_WIDTH;

      }

      *str_char_ptr++ = *p++;
      str_curr_hdr->s_length++;

   }
}

/*\
 *  \function{setl2\_any()}
 *
 *  This function is the \verb"any" built-in function.  We split one
 *  input string into two: the first character if it is in the span
 *  set and everything remaining.
\*/

void setl2_any(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length;                   /* source string length              */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
char *p;                               /* temporary looping variable        */

   /* make sure both arguments are strings */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"any",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"any",
            abend_opnd_str(SETL_SYSTEM argv+1));

   /* initialize the character set to empty */

   for (p = character_set; p < character_set + 256; *p++ = NO);

   /* set to true all the characters in the pattern string */

   source_hdr = argv[1].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;

   for (source_cell = source_hdr->s_head;
        source_cell != NULL;
        source_cell = source_cell->s_next) {

      for (p = source_cell->s_cell_value;
           p < source_cell->s_cell_value + STR_CELL_WIDTH &&
              p < source_cell->s_cell_value + source_length;
           character_set[(unsigned char)(*p++)] = YES);

      source_length -= STR_CELL_WIDTH;

   }

   /* check the first character of the source string */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   source_cell = source_hdr->s_head;
   if (source_cell != NULL &&
       source_hdr->s_length >= 1 &&
       character_set[(unsigned char)(source_cell->s_cell_value[0])]) {

      /* we found a match, create a one character string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = 1;
      get_string_cell(target_cell);
      target_hdr->s_head = target_hdr->s_tail = target_cell;
      target_cell->s_next = target_cell->s_prev = NULL;
      target_cell->s_cell_value[0] = source_cell->s_cell_value[0];

      /* the first string is assigned to the target */

      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = target_hdr;

      /* set up to copy the tail of the string */

      source_length = source_hdr->s_length - 1;
      source_char_ptr = source_cell->s_cell_value + 1;
      source_char_end = source_cell->s_cell_value + STR_CELL_WIDTH;

      /* next create a tail string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = source_length;
      target_hdr->s_head = target_hdr->s_tail = NULL;
      target_char_ptr = target_char_end = NULL;

      /* copy the tail of the source string */

      while (source_length--) {

         if (source_char_ptr == source_char_end) {

            source_cell = source_cell->s_next;
            source_char_ptr = source_cell->s_cell_value;
            source_char_end = source_char_ptr + STR_CELL_WIDTH;

         }

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

         *target_char_ptr++ = *source_char_ptr++;

      }

      /* push the tail string */

      source.sp_form = ft_string;
      source.sp_val.sp_string_ptr = target_hdr;
      push_pstack(&source);
      target_hdr->s_use_count--;

   }

   /*
    *  If we didn't match, the left string is null and the right is the
    *  source.
    */

   else {

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = 0;
      target_hdr->s_head = target_hdr->s_tail = NULL;

      /* the first string is assigned to the target */

      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = target_hdr;

      /* push the tail string */

      push_pstack(argv);

   }

   return;

}

/*\
 *  \function{setl2\_break()}
 *
 *  This function is the \verb"break" built-in function.  We split one
 *  input string into two: an initial string all of whose characters
 *  do not belong to an input set, and everything remaining.
\*/

void setl2_break(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length;                   /* source string length              */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
char *p;                               /* temporary looping variable        */

   /* make sure both arguments are strings */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"break",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"break",
            abend_opnd_str(SETL_SYSTEM argv+1));

   /* initialize the character set to empty */

   for (p = character_set; p < character_set + 256; *p++ = NO);

   /* set to true all the characters in the pattern string */

   source_hdr = argv[1].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;

   for (source_cell = source_hdr->s_head;
        source_cell != NULL;
        source_cell = source_cell->s_next) {

      for (p = source_cell->s_cell_value;
           p < source_cell->s_cell_value + STR_CELL_WIDTH &&
              p < source_cell->s_cell_value + source_length;
           character_set[(unsigned char)(*p++)] = YES);

      source_length -= STR_CELL_WIDTH;

   }

   /* initialize the source string */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;
   source_cell = source_hdr->s_head;
   if (source_cell == NULL) {
      source_char_ptr = source_char_end = NULL;
   }
   else {
      source_char_ptr = source_cell->s_cell_value;
      source_char_end = source_char_ptr + STR_CELL_WIDTH;
   }

   /* make a target string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = 0;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the source until we find something not in the span set */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

      if (character_set[(unsigned char)(*source_char_ptr)])
         break;

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* we decremented the length once too often */

   source_length++;
   target_hdr->s_length = source_hdr->s_length - source_length;

   /* the first string is assigned to the target */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;

   /* next create a tail string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = source_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the tail of the source string */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* push the tail string */

   source.sp_form = ft_string;
   source.sp_val.sp_string_ptr = target_hdr;
   push_pstack(&source);
   target_hdr->s_use_count--;

   return;

}

/*\
 *  \function{setl2\_len()}
 *
 *  This function is the \verb"len" built-in function.  We split split
 *  the source string into two: an initial string of {\em n} characters,
 *  and everything else.
\*/

void setl2_len(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length, target_length;    /* source and target lengths         */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */

   /* verify the argument types */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"len",
            abend_opnd_str(SETL_SYSTEM argv));

   /* we convert the length to a C long */

   if (argv[1].sp_form == ft_short) {

      target_length = argv[1].sp_val.sp_short_value;
      if (target_length <= 0)
         abend(SETL_SYSTEM msg_bad_arg,"non-negative integer",2,"len",
               abend_opnd_str(SETL_SYSTEM argv+1));

   }
   else if (argv[1].sp_form == ft_long) {

      if ((argv[1].sp_val.sp_long_ptr)->i_is_negative)
         abend(SETL_SYSTEM msg_bad_arg,"non-negative integer",2,"len",
               abend_opnd_str(SETL_SYSTEM argv+1));

      target_length = long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);

   }
   else {

      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"len",
            abend_opnd_str(SETL_SYSTEM argv+1));

   }

   /* initialize the source string */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;
   source_cell = source_hdr->s_head;
   if (source_cell == NULL) {
      source_char_ptr = source_char_end = NULL;
   }
   else {
      source_char_ptr = source_cell->s_cell_value;
      source_char_end = source_char_ptr + STR_CELL_WIDTH;
   }

   /* adjust the target length */

   if (target_length > source_length) {
      target_length = source_length;
      source_length = 0;
   }
   else {
      source_length -= target_length;
   }

   /* make a target string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = target_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the first n characters from the source */

   while (target_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* the first string is assigned to the target */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;

   /* next create a tail string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = source_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the tail of the source string */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* push the tail string */

   source.sp_form = ft_string;
   source.sp_val.sp_string_ptr = target_hdr;
   push_pstack(&source);
   target_hdr->s_use_count--;

   return;

}

/*\
 *  \function{setl2\_match()}
 *
 *  This function is the \verb"match" built-in function.  We check that
 *  the pattern string matches the initial substring of the input string.
\*/

void setl2_match(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length, target_length;    /* source and target lengths         */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */

   /* make sure both arguments are strings */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"match",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"match",
            abend_opnd_str(SETL_SYSTEM argv+1));

   /* set up to scan each string */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   target_hdr = argv[1].sp_val.sp_string_ptr;

   if (target_hdr->s_length <= source_hdr->s_length) {

      /* initialize each string */

      source_length = source_hdr->s_length - target_hdr->s_length;
      source_cell = source_hdr->s_head;
      if (source_cell == NULL) {
         source_char_ptr = source_char_end = NULL;
      }
      else {
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;
      }

      target_length = target_hdr->s_length;
      target_cell = target_hdr->s_head;
      if (target_cell == NULL) {
         target_char_ptr = target_char_end = NULL;
      }
      else {
         target_char_ptr = target_cell->s_cell_value;
         target_char_end = target_char_ptr + STR_CELL_WIDTH;
      }

      /* compare the initial substrings */

      while (target_length--) {

         if (source_char_ptr == source_char_end) {

            source_cell = source_cell->s_next;
            source_char_ptr = source_cell->s_cell_value;
            source_char_end = source_char_ptr + STR_CELL_WIDTH;

         }

         if (target_char_ptr == target_char_end) {

            target_cell = target_cell->s_next;
            target_char_ptr = target_cell->s_cell_value;
            target_char_end = target_char_ptr + STR_CELL_WIDTH;

         }

         if (*source_char_ptr != *target_char_ptr)
            break;

         target_char_ptr++;
         source_char_ptr++;

      }

      /* if the target matches the source */

      if (target_length < 0) {

         /* the first string is assigned to the target */

         target_hdr->s_use_count++;
         unmark_specifier(target);
         target->sp_form = ft_string;
         target->sp_val.sp_string_ptr = target_hdr;

         /* next create a tail string */

         get_string_header(target_hdr);
         target_hdr->s_use_count = 1;
         target_hdr->s_hash_code = -1;
         target_hdr->s_length = source_length;
         target_hdr->s_head = target_hdr->s_tail = NULL;
         target_char_ptr = target_char_end = NULL;

         /* copy the tail of the source string */

         while (source_length--) {

            if (source_char_ptr == source_char_end) {

               source_cell = source_cell->s_next;
               source_char_ptr = source_cell->s_cell_value;
               source_char_end = source_char_ptr + STR_CELL_WIDTH;

            }

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

            *target_char_ptr++ = *source_char_ptr++;

         }

         /* push the tail string */

         source.sp_form = ft_string;
         source.sp_val.sp_string_ptr = target_hdr;
         push_pstack(&source);
         target_hdr->s_use_count--;

         return;

      }
   }

   /*
    *  At this point the match has failed.  We return a null string and
    *  the initial source.
    */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = 0;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* the first string is assigned to the target */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;

   /* push the source string */

   push_pstack(argv);

   return;

}

/*\
 *  \function{setl2\_notany()}
 *
 *  This function is the \verb"notany" built-in function.  We split one
 *  input string into two: the first character if it is not in the span
 *  set and everything remaining.
\*/

void setl2_notany(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length;                   /* source string length              */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
char *p;                               /* temporary looping variable        */

   /* make sure both arguments are strings */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"notany",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"notany",
            abend_opnd_str(SETL_SYSTEM argv+1));

   /* initialize the character set to empty */

   for (p = character_set; p < character_set + 256; *p++ = NO);

   /* set to true all the characters in the pattern string */

   source_hdr = argv[1].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;

   for (source_cell = source_hdr->s_head;
        source_cell != NULL;
        source_cell = source_cell->s_next) {

      for (p = source_cell->s_cell_value;
           p < source_cell->s_cell_value + STR_CELL_WIDTH &&
              p < source_cell->s_cell_value + source_length;
           character_set[(unsigned char)(*p++)] = YES);

      source_length -= STR_CELL_WIDTH;

   }

   /* check the first character of the source string */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   source_cell = source_hdr->s_head;
   if (source_cell != NULL &&
       source_hdr->s_length >= 1 &&
       !character_set[(unsigned char)(source_cell->s_cell_value[0])]) {

      /* we found a match, create a one character string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = 1;
      get_string_cell(target_cell);
      target_hdr->s_head = target_hdr->s_tail = target_cell;
      target_cell->s_next = target_cell->s_prev = NULL;
      target_cell->s_cell_value[0] = source_cell->s_cell_value[0];

      /* the first string is assigned to the target */

      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = target_hdr;

      /* set up to copy the tail of the string */

      source_length = source_hdr->s_length - 1;
      source_char_ptr = source_cell->s_cell_value + 1;
      source_char_end = source_cell->s_cell_value + STR_CELL_WIDTH;

      /* next create a tail string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = source_length;
      target_hdr->s_head = target_hdr->s_tail = NULL;
      target_char_ptr = target_char_end = NULL;

      /* copy the tail of the source string */

      while (source_length--) {

         if (source_char_ptr == source_char_end) {

            source_cell = source_cell->s_next;
            source_char_ptr = source_cell->s_cell_value;
            source_char_end = source_char_ptr + STR_CELL_WIDTH;

         }

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

         *target_char_ptr++ = *source_char_ptr++;

      }

      /* push the tail string */

      source.sp_form = ft_string;
      source.sp_val.sp_string_ptr = target_hdr;
      push_pstack(&source);
      target_hdr->s_use_count--;

   }

   /*
    *  If we didn't match, the left string is null and the right is the
    *  source.
    */

   else {

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = 0;
      target_hdr->s_head = target_hdr->s_tail = NULL;

      /* the first string is assigned to the target */

      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = target_hdr;

      /* push the source string */

      push_pstack(argv);

   }

   return;

}

#endif                                 /* ROOT                              */
#if !SHORT_TEXT | PARTA

/*\
 *  \function{setl2\_span()}
 *
 *  This function is the \verb"span" built-in function.  We split one
 *  input string into two: an initial string all of whose characters
 *  belong to an input set, and everything remaining.
\*/

void setl2_span(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length;                   /* source string length              */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
char *p;                               /* temporary looping variable        */

   /* make sure both arguments are strings */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"span",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"span",
            abend_opnd_str(SETL_SYSTEM argv+1));

   /* initialize the character set to empty */

   for (p = character_set; p < character_set + 256; *p++ = NO);

   /* set to true all the characters in the pattern string */

   source_hdr = argv[1].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;

   for (source_cell = source_hdr->s_head;
        source_cell != NULL;
        source_cell = source_cell->s_next) {

      for (p = source_cell->s_cell_value;
           p < source_cell->s_cell_value + STR_CELL_WIDTH &&
              p < source_cell->s_cell_value + source_length;
           character_set[(unsigned char)(*p++)] = YES);

      source_length -= STR_CELL_WIDTH;

   }

   /* initialize the source string */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;
   source_cell = source_hdr->s_head;
   if (source_cell == NULL) {
      source_char_ptr = source_char_end = NULL;
   }
   else {
      source_char_ptr = source_cell->s_cell_value;
      source_char_end = source_char_ptr + STR_CELL_WIDTH;
   }

   /* make a target string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = 0;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the source until we find something not in the span set */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

      if (!character_set[(unsigned char)(*source_char_ptr)])
         break;

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* we decremented the length once too often */

   source_length++;
   target_hdr->s_length = source_hdr->s_length - source_length;

   /* the first string is assigned to the target */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;

   /* next create a tail string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = source_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the tail of the source string */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* push the tail string */

   source.sp_form = ft_string;
   source.sp_val.sp_string_ptr = target_hdr;
   push_pstack(&source);
   target_hdr->s_use_count--;

   return;

}

/*\
 *  \function{setl2\_lpad()}
 *
 *  This function is the \verb"lpad" built-in function.  We pad the input
 *  string with blanks on the left.
\*/

void setl2_lpad(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length, target_length;    /* and target lengths                */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */

   /* verify the argument types */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"lpad",
            abend_opnd_str(SETL_SYSTEM argv));

   /* we convert the length to a C long */

   if (argv[1].sp_form == ft_short) {

      target_length = argv[1].sp_val.sp_short_value;
      if (target_length <= 0)
         abend(SETL_SYSTEM msg_bad_arg,"non-negative integer",2,"lpad",
               abend_opnd_str(SETL_SYSTEM argv+1));


   }
   else if (argv[1].sp_form == ft_long) {

      if ((argv[1].sp_val.sp_long_ptr)->i_is_negative)
         abend(SETL_SYSTEM msg_bad_arg,"non-negative integer",2,"lpad",
               abend_opnd_str(SETL_SYSTEM argv+1));

      target_length = long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);

   }
   else {

      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"lpad",
            abend_opnd_str(SETL_SYSTEM argv+1));

   }

   /* if the length is already too long return the argument */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   if (source_hdr->s_length >= target_length) {

      source_hdr->s_use_count++;
      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = source_hdr;

      return;

   }

   /* make a target string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = target_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* set up the source string */

   source_length = source_hdr->s_length;
   source_cell = source_hdr->s_head;
   if (source_cell == NULL) {
      source_char_ptr = source_char_end = NULL;
   }
   else {
      source_char_ptr = source_cell->s_cell_value;
      source_char_end = source_char_ptr + STR_CELL_WIDTH;
   }

   /* pad the target with blanks */

   target_length -= source_length;
   while (target_length--) {

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

      *target_char_ptr++ = ' ';

   }

   /* copy the source */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;

   return;

}

/*\
 *  \function{setl2\_rany()}
 *
 *  This function is the \verb"rany" built-in function.  We split one
 *  input string into two: the last character if it is in the span
 *  set and everything remaining.
\*/

void setl2_rany(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length;                   /* source string length              */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
char *p;                               /* temporary looping variable        */

   /* make sure both arguments are strings */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"rany",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"rany",
            abend_opnd_str(SETL_SYSTEM argv+1));

   /* initialize the character set to empty */

   for (p = character_set; p < character_set + 256; *p++ = NO);

   /* set to true all the characters in the pattern string */

   source_hdr = argv[1].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;

   for (source_cell = source_hdr->s_head;
        source_cell != NULL;
        source_cell = source_cell->s_next) {

      for (p = source_cell->s_cell_value;
           p < source_cell->s_cell_value + STR_CELL_WIDTH &&
              p < source_cell->s_cell_value + source_length;
           character_set[(unsigned char)(*p++)] = YES);

      source_length -= STR_CELL_WIDTH;

   }

   /* check the first character of the source string */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   source_cell = source_hdr->s_tail;
   if (source_cell != NULL) {
      source_char_ptr = source_cell->s_cell_value - 1 +
                        (source_hdr->s_length % STR_CELL_WIDTH);
      if (source_char_ptr < source_cell->s_cell_value)
         source_char_ptr += STR_CELL_WIDTH;
   }

   if (source_cell != NULL &&
       source_hdr->s_length >= 1 &&
       character_set[(unsigned char)(*source_char_ptr)]) {

      /* we found a match, create a one character string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = 1;
      get_string_cell(target_cell);
      target_hdr->s_head = target_hdr->s_tail = target_cell;
      target_cell->s_next = target_cell->s_prev = NULL;
      target_cell->s_cell_value[0] = *source_char_ptr;

      /* the first string is assigned to the target */

      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = target_hdr;

      /* set up to copy the tail of the string */

      source_cell = source_hdr->s_head;
      source_length = source_hdr->s_length - 1;
      source_char_ptr = source_cell->s_cell_value;
      source_char_end = source_cell->s_cell_value + STR_CELL_WIDTH;

      /* next create a tail string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = source_length;
      target_hdr->s_head = target_hdr->s_tail = NULL;
      target_char_ptr = target_char_end = NULL;

      /* copy the tail of the source string */

      while (source_length--) {

         if (source_char_ptr == source_char_end) {

            source_cell = source_cell->s_next;
            source_char_ptr = source_cell->s_cell_value;
            source_char_end = source_char_ptr + STR_CELL_WIDTH;

         }

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

         *target_char_ptr++ = *source_char_ptr++;

      }

      /* push the tail string */

      source.sp_form = ft_string;
      source.sp_val.sp_string_ptr = target_hdr;
      push_pstack(&source);
      target_hdr->s_use_count--;

   }

   /*
    *  If we didn't match, the left string is null and the right is the
    *  source.
    */

   else {

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = 0;
      target_hdr->s_head = target_hdr->s_tail = NULL;

      /* the first string is assigned to the target */

      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = target_hdr;

      /* push the source string */

      push_pstack(argv);

   }

   return;

}

/*\
 *  \function{setl2\_rbreak()}
 *
 *  This function is the \verb"rbreak" built-in function.  We split one
 *  input string into two: an end string all of whose characters do not
 *  belong to an input set, and everything remaining.
\*/

void setl2_rbreak(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length, target_length;    /* and target lengths                */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
char *p;                               /* temporary looping variable        */

   /* make sure both arguments are strings */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"rbreak",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"rbreak",
            abend_opnd_str(SETL_SYSTEM argv+1));

   /* initialize the character set to empty */

   for (p = character_set; p < character_set + 256; *p++ = NO);

   /* set to true all the characters in the pattern string */

   source_hdr = argv[1].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;

   for (source_cell = source_hdr->s_head;
        source_cell != NULL;
        source_cell = source_cell->s_next) {

      for (p = source_cell->s_cell_value;
           p < source_cell->s_cell_value + STR_CELL_WIDTH &&
              p < source_cell->s_cell_value + source_length;
           character_set[(unsigned char)(*p++)] = YES);

      source_length -= STR_CELL_WIDTH;

   }

   /* initialize the source string for scanning from right */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;
   source_cell = source_hdr->s_tail;
   if (source_cell == NULL) {
      source_char_ptr = source_char_end = NULL;
   }
   else {
      source_char_end = source_cell->s_cell_value - 1;
      source_char_ptr = source_char_end +
                        (source_hdr->s_length % STR_CELL_WIDTH);
      if (source_char_end == source_char_ptr)
         source_char_ptr += STR_CELL_WIDTH;
   }

   /* search for a character not in the span set */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_prev;
         source_char_end = source_cell->s_cell_value - 1;
         source_char_ptr = source_char_end + STR_CELL_WIDTH;

      }

      if (character_set[(unsigned char)(*source_char_ptr)])
         break;

      source_char_ptr--;

   }

   /* we decremented the length once too often */

   source_length++;
   source_char_ptr++;
   target_length = source_hdr->s_length - source_length;
   if (source_cell != NULL)
      source_char_end = source_cell->s_cell_value + STR_CELL_WIDTH;

   /* make a target string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = target_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the source to the end */

   while (target_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* the first string is assigned to the target */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;

   /* next create a tail string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = source_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* reset the source to the start */

   source_cell = source_hdr->s_head;
   if (source_cell == NULL) {
      source_char_ptr = source_char_end = NULL;
   }
   else {
      source_char_ptr = source_cell->s_cell_value;
      source_char_end = source_char_ptr + STR_CELL_WIDTH;
   }

   /* copy the tail of the source string */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* push the tail string */

   source.sp_form = ft_string;
   source.sp_val.sp_string_ptr = target_hdr;
   push_pstack(&source);
   target_hdr->s_use_count--;

   return;

}

/*\
 *  \function{setl2\_rlen()}
 *
 *  This function is the \verb"rlen" built-in function.  We split split
 *  the source string into two: a final string of {\em n} characters, and
 *  everything else.
\*/

void setl2_rlen(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length, target_length;    /* source and target lengths         */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */

   /* verify the argument types */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"rlen",
            abend_opnd_str(SETL_SYSTEM argv));

   /* we convert the length to a C long */

   if (argv[1].sp_form == ft_short) {

      target_length = argv[1].sp_val.sp_short_value;
      if (target_length <= 0)
         abend(SETL_SYSTEM msg_bad_arg,"non-negative integer",2,"rlen",
               abend_opnd_str(SETL_SYSTEM argv+1));

   }
   else if (argv[1].sp_form == ft_long) {

      if ((argv[1].sp_val.sp_long_ptr)->i_is_negative)
         abend(SETL_SYSTEM msg_bad_arg,"non-negative integer",2,"rlen",
               abend_opnd_str(SETL_SYSTEM argv+1));

      target_length = long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);

   }
   else {

      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"rlen",
            abend_opnd_str(SETL_SYSTEM argv+1));

   }

   /* initialize the source string */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;
   source_cell = source_hdr->s_head;
   if (source_cell == NULL) {
      source_char_ptr = source_char_end = NULL;
   }
   else {
      source_char_ptr = source_cell->s_cell_value;
      source_char_end = source_char_ptr + STR_CELL_WIDTH;
   }

   /* adjust the target length */

   if (target_length > source_length) {
      target_length = source_length;
      source_length = 0;
   }
   else {
      source_length -= target_length;
   }

   /* make a target string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = source_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the first length - n characters from the source */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* push the first string */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;
   push_pstack(target);
   target_hdr->s_use_count--;

   /* next create a tail string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = target_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the tail of the source string */

   while (target_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* the second string is assigned to the target */

   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;

   return;

}

/*\
 *  \function{setl2\_rmatch()}
 *
 *  This function is the \verb"rmatch" built-in function.  We check that
 *  the pattern string matches the final substring of the input string.
\*/

void setl2_rmatch(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length, target_length;    /* source and target lengths         */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */

   /* make sure both arguments are strings */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"rmatch",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"rmatch",
            abend_opnd_str(SETL_SYSTEM argv+1));

   /* set up to scan each string */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   target_hdr = argv[1].sp_val.sp_string_ptr;

   if (target_hdr->s_length <= source_hdr->s_length) {

      /* initialize each string */

      source_length = source_hdr->s_length - target_hdr->s_length;
      source_cell = source_hdr->s_head;
      while (source_length >= STR_CELL_WIDTH) {

         source_length -= STR_CELL_WIDTH;
         source_cell = source_cell->s_next;

      }
      if (source_cell == NULL) {
         source_char_ptr = source_char_end = NULL;
      }
      else {
         source_char_ptr = source_cell->s_cell_value + source_length;
         source_char_end = source_cell->s_cell_value + STR_CELL_WIDTH;
      }
      source_length = source_hdr->s_length - target_hdr->s_length;

      target_length = target_hdr->s_length;
      target_cell = target_hdr->s_head;
      if (target_cell == NULL) {
         target_char_ptr = target_char_end = NULL;
      }
      else {
         target_char_ptr = target_cell->s_cell_value;
         target_char_end = target_char_ptr + STR_CELL_WIDTH;
      }

      /* compare the final substrings */

      while (target_length--) {

         if (source_char_ptr == source_char_end) {

            source_cell = source_cell->s_next;
            source_char_ptr = source_cell->s_cell_value;
            source_char_end = source_char_ptr + STR_CELL_WIDTH;

         }

         if (target_char_ptr == target_char_end) {

            target_cell = target_cell->s_next;
            target_char_ptr = target_cell->s_cell_value;
            target_char_end = target_char_ptr + STR_CELL_WIDTH;

         }

         if (*source_char_ptr != *target_char_ptr)
            break;

         target_char_ptr++;
         source_char_ptr++;

      }

      /* if the target matches the source */

      if (target_length < 0) {

         /* the first string is assigned to the target */

         target_hdr->s_use_count++;
         unmark_specifier(target);
         target->sp_form = ft_string;
         target->sp_val.sp_string_ptr = target_hdr;

         /* next create a leading string */

         get_string_header(target_hdr);
         target_hdr->s_use_count = 1;
         target_hdr->s_hash_code = -1;
         target_hdr->s_length = source_length;
         target_hdr->s_head = target_hdr->s_tail = NULL;
         target_char_ptr = target_char_end = NULL;

         /* reset the source to the beginning */

         source_cell = source_hdr->s_head;
         if (source_cell == NULL) {
            source_char_ptr = source_char_end = NULL;
         }
         else {
            source_char_ptr = source_cell->s_cell_value;
            source_char_end = source_char_ptr + STR_CELL_WIDTH;
         }

         /* copy the front of the source string */

         while (source_length--) {

            if (source_char_ptr == source_char_end) {

               source_cell = source_cell->s_next;
               source_char_ptr = source_cell->s_cell_value;
               source_char_end = source_char_ptr + STR_CELL_WIDTH;

            }

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

            *target_char_ptr++ = *source_char_ptr++;

         }

         /* push the tail string */

         source.sp_form = ft_string;
         source.sp_val.sp_string_ptr = target_hdr;
         push_pstack(&source);
         target_hdr->s_use_count--;

         return;

      }
   }

   /*
    *  At this point the match has failed.  We return a null string and
    *  the initial source.
    */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = 0;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* the first string is assigned to the target */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;

   /* push the source string */

   push_pstack(argv);

   return;

}

/*\
 *  \function{setl2\_rnotany()}
 *
 *  This function is the \verb"rnotany" built-in function.  We split one
 *  input string into two: the last character if it is not in the span
 *  set and everything remaining.
\*/

void setl2_rnotany(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length;                   /* source string length              */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
char *p;                               /* temporary looping variable        */

   /* make sure both arguments are strings */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"rnotany",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"rnotany",
            abend_opnd_str(SETL_SYSTEM argv+1));

   /* initialize the character set to empty */

   for (p = character_set; p < character_set + 256; *p++ = NO);

   /* set to true all the characters in the pattern string */

   source_hdr = argv[1].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;

   for (source_cell = source_hdr->s_head;
        source_cell != NULL;
        source_cell = source_cell->s_next) {

      for (p = source_cell->s_cell_value;
           p < source_cell->s_cell_value + STR_CELL_WIDTH &&
              p < source_cell->s_cell_value + source_length;
           character_set[(unsigned char)(*p++)] = YES);

      source_length -= STR_CELL_WIDTH;

   }

   /* check the first character of the source string */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   source_cell = source_hdr->s_tail;
   if (source_cell != NULL) {
      source_char_ptr = source_cell->s_cell_value - 1 +
                        (source_hdr->s_length % STR_CELL_WIDTH);
      if (source_char_ptr < source_cell->s_cell_value)
         source_char_ptr += STR_CELL_WIDTH;
   }

   if (source_cell != NULL &&
       source_hdr->s_length >= 1 &&
       !character_set[(unsigned char)(*source_char_ptr)]) {

      /* we found a match, create a one character string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = 1;
      get_string_cell(target_cell);
      target_hdr->s_head = target_hdr->s_tail = target_cell;
      target_cell->s_next = target_cell->s_prev = NULL;
      target_cell->s_cell_value[0] = *source_char_ptr;

      /* the first string is assigned to the target */

      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = target_hdr;

      /* set up to copy the tail of the string */

      source_cell = source_hdr->s_head;
      source_length = source_hdr->s_length - 1;
      source_char_ptr = source_cell->s_cell_value;
      source_char_end = source_cell->s_cell_value + STR_CELL_WIDTH;

      /* next create a tail string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = source_length;
      target_hdr->s_head = target_hdr->s_tail = NULL;
      target_char_ptr = target_char_end = NULL;

      /* copy the tail of the source string */

      while (source_length--) {

         if (source_char_ptr == source_char_end) {

            source_cell = source_cell->s_next;
            source_char_ptr = source_cell->s_cell_value;
            source_char_end = source_char_ptr + STR_CELL_WIDTH;

         }

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

         *target_char_ptr++ = *source_char_ptr++;

      }

      /* push the tail string */

      source.sp_form = ft_string;
      source.sp_val.sp_string_ptr = target_hdr;
      push_pstack(&source);
      target_hdr->s_use_count--;

   }

   /*
    *  If we didn't match, the left string is null and the right is the
    *  source.
    */

   else {

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = 0;
      target_hdr->s_head = target_hdr->s_tail = NULL;

      /* the first string is assigned to the target */

      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = target_hdr;

      /* push the source string */

      push_pstack(argv);

   }

   return;

}

/*\
 *  \function{setl2\_rspan()}
 *
 *  This function is the \verb"rspan" built-in function.  We split one
 *  input string into two: an end string all of whose characters belong
 *  to an input set, and everything remaining.
\*/

void setl2_rspan(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
specifier source;                      /* returned source string            */
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length, target_length;    /* and target lengths                */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
char *p;                               /* temporary looping variable        */

   /* make sure both arguments are strings */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"rspan",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"rspan",
            abend_opnd_str(SETL_SYSTEM argv+1));

   /* initialize the character set to empty */

   for (p = character_set; p < character_set + 256; *p++ = NO);

   /* set to true all the characters in the pattern string */

   source_hdr = argv[1].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;

   for (source_cell = source_hdr->s_head;
        source_cell != NULL;
        source_cell = source_cell->s_next) {

      for (p = source_cell->s_cell_value;
           p < source_cell->s_cell_value + STR_CELL_WIDTH &&
              p < source_cell->s_cell_value + source_length;
           character_set[(unsigned char)(*p++)] = YES);

      source_length -= STR_CELL_WIDTH;

   }

   /* initialize the source string for scanning from right */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   source_length = source_hdr->s_length;
   source_cell = source_hdr->s_tail;
   if (source_cell == NULL) {
      source_char_ptr = source_char_end = NULL;
   }
   else {
      source_char_end = source_cell->s_cell_value - 1;
      source_char_ptr = source_char_end +
                        (source_hdr->s_length % STR_CELL_WIDTH);
      if (source_char_end == source_char_ptr)
         source_char_ptr += STR_CELL_WIDTH;
   }

   /* search for a character not in the span set */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_prev;
         source_char_end = source_cell->s_cell_value - 1;
         source_char_ptr = source_char_end + STR_CELL_WIDTH;

      }

      if (!character_set[(unsigned char)(*source_char_ptr)])
         break;

      source_char_ptr--;

   }

   /* we decremented the length once too often */

   source_length++;
   source_char_ptr++;
   target_length = source_hdr->s_length - source_length;
   if (source_cell != NULL)
      source_char_end = source_cell->s_cell_value + STR_CELL_WIDTH;

   /* make a target string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = target_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the source to the end */

   while (target_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* the first string is assigned to the target */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;

   /* next create a tail string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = source_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* reset the source to the start */

   source_cell = source_hdr->s_head;
   if (source_cell == NULL) {
      source_char_ptr = source_char_end = NULL;
   }
   else {
      source_char_ptr = source_cell->s_cell_value;
      source_char_end = source_char_ptr + STR_CELL_WIDTH;
   }

   /* copy the tail of the source string */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* push the tail string */

   source.sp_form = ft_string;
   source.sp_val.sp_string_ptr = target_hdr;
   push_pstack(&source);
   target_hdr->s_use_count--;

   return;

}

/*\
 *  \function{setl2\_rpad()}
 *
 *  This function is the \verb"rpad" built-in function.  We pad the input
 *  string with blanks on the right.
\*/

void setl2_rpad(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type source_hdr, target_hdr;
                                       /* source and target strings         */
string_c_ptr_type source_cell, target_cell;
                                       /* source and target string cells    */
int32 source_length, target_length;    /* and target lengths                */
char *source_char_ptr, *source_char_end;
                                       /* source string pointers            */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */

   /* verify the argument types */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"rpad",
            abend_opnd_str(SETL_SYSTEM argv));

   /* we convert the length to a C long */

   if (argv[1].sp_form == ft_short) {

      target_length = argv[1].sp_val.sp_short_value;
      if (target_length <= 0)
         abend(SETL_SYSTEM msg_bad_arg,"non-negative integer",2,"rpad",
               abend_opnd_str(SETL_SYSTEM argv+1));

   }
   else if (argv[1].sp_form == ft_long) {

      if ((argv[1].sp_val.sp_long_ptr)->i_is_negative)
         abend(SETL_SYSTEM msg_bad_arg,"non-negative integer",2,"rpad",
               abend_opnd_str(SETL_SYSTEM argv+1));

      target_length = long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);

   }
   else {

      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"rpad",
            abend_opnd_str(SETL_SYSTEM argv+1));

   }

   /* if the length is already too long return the argument */

   source_hdr = argv[0].sp_val.sp_string_ptr;
   if (source_hdr->s_length >= target_length) {

      source_hdr->s_use_count++;
      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = source_hdr;

      return;

   }

   /* make a target string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = target_length;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* set up the source string */

   source_length = source_hdr->s_length;
   source_cell = source_hdr->s_head;
   if (source_cell == NULL) {
      source_char_ptr = source_char_end = NULL;
   }
   else {
      source_char_ptr = source_cell->s_cell_value;
      source_char_end = source_char_ptr + STR_CELL_WIDTH;
   }
   target_length -= source_length;

   /* copy the source */

   while (source_length--) {

      if (source_char_ptr == source_char_end) {

         source_cell = source_cell->s_next;
         source_char_ptr = source_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

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

      *target_char_ptr++ = *source_char_ptr++;

   }

   /* pad the target with blanks */

   while (target_length--) {

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

      *target_char_ptr++ = ' ';

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_hdr;

   return;

}

#endif                                 /* PARTA                             */
