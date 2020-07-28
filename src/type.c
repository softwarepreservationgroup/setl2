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
 *  \package{Type Checking Procedures}
 *
 *  In this package we have built-in procedures designed to check
 *  specifier types.  For the time being we have provided all the
 *  procedures in SETL, but we may discard some of these later.
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "builtins.h"                  /* built-in symbols                  */
#include "unittab.h"                   /* unit table                        */
#include "specs.h"                     /* specifiers                        */
#include "x_strngs.h"                  /* strings                           */
#include "maps.h"                      /* maps                              */
#include "objects.h"                   /* objects                           */


/*\
 *  \function{setl2\_type()}
 *
 *  This function is the \verb"type" built-in function.  We just return a
 *  character string representing the form of the argument.
\*/

void setl2_type(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* string header pointer             */
string_c_ptr_type string_cell;         /* string cell pointer               */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
char *p;                               /* temporary looping variable        */
static object_h_ptr_type object_root;  /* object header pointer             */
static unittab_ptr_type class_ptr;     /* class pointer                     */

   /* omega returns om */

   if (argv[0].sp_form == ft_omega) {

      unmark_specifier(target);
      target->sp_form = ft_omega;

      return;

   }

   /* create a string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   get_string_cell(string_cell);
   string_cell->s_next = string_cell->s_prev = NULL;
   string_hdr->s_head = string_hdr->s_tail = string_cell;

   /*
    *  We set the text of the string.  Note that all of these character
    *  strings will fit in a single cell.
    */

   switch (argv[0].sp_form) {

      case ft_atom :

         if (argv[0].sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num ||
             argv[0].sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num) {

            strcpy(string_cell->s_cell_value,"BOOLEAN");

         }
         else {

            strcpy(string_cell->s_cell_value,"ATOM");

         }

         string_hdr->s_length = strlen(string_cell->s_cell_value);

         break;

      case ft_short :         case ft_long :

         strcpy(string_cell->s_cell_value,"INTEGER");
         string_hdr->s_length = strlen(string_cell->s_cell_value);
         break;

      case ft_real :

         strcpy(string_cell->s_cell_value,"REAL");
         string_hdr->s_length = strlen(string_cell->s_cell_value);
         break;

      case ft_string :

         strcpy(string_cell->s_cell_value,"STRING");
         string_hdr->s_length = strlen(string_cell->s_cell_value);
         break;

      case ft_set :           case ft_map :

         strcpy(string_cell->s_cell_value,"SET");
         string_hdr->s_length = strlen(string_cell->s_cell_value);
         break;

      case ft_tuple :

         strcpy(string_cell->s_cell_value,"TUPLE");
         string_hdr->s_length = strlen(string_cell->s_cell_value);
         break;

      case ft_proc :

         strcpy(string_cell->s_cell_value,"PROCEDURE");
         string_hdr->s_length = strlen(string_cell->s_cell_value);
         break;

      case ft_mailbox :

         strcpy(string_cell->s_cell_value,"MAILBOX");
         string_hdr->s_length = strlen(string_cell->s_cell_value);
         break;

      case ft_object :
      case ft_process :

         /* pick out the object root and class */

         object_root = argv[0].sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* set up the target pointers */

         target_char_ptr = string_cell->s_cell_value;
         target_char_end = target_char_ptr + STR_CELL_WIDTH;
         string_hdr->s_length = 0;

         for (p = class_ptr->ut_name; *p; p++) {

            if (target_char_ptr == target_char_end) {

               get_string_cell(string_cell);
               if (string_hdr->s_tail != NULL)
                  (string_hdr->s_tail)->s_next = string_cell;
               string_cell->s_prev = string_hdr->s_tail;
               string_cell->s_next = NULL;
               string_hdr->s_tail = string_cell;
               if (string_hdr->s_head == NULL)
                  string_hdr->s_head = string_cell;
               target_char_ptr = string_cell->s_cell_value;
               target_char_end = target_char_ptr + STR_CELL_WIDTH;

            }

            *target_char_ptr++ = *p;
            string_hdr->s_length++;

         }

   }

   /* set the target and return */


   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;

}

/*\
 *  \function{setl2\_is\_atom()}
 *
 *  This function is the \verb"is_atom" built-in function.  We return true
 *  if the argument is an atom, and false otherwise.
\*/

void setl2_is_atom(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   if (argv[0].sp_form == ft_atom &&
       argv[0].sp_val.sp_atom_num != spec_true->sp_val.sp_atom_num &&
       argv[0].sp_val.sp_atom_num != spec_false->sp_val.sp_atom_num) {

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
}

/*\
 *  \function{setl2\_is\_boolean()}
 *
 *  This function is the \verb"is_boolean" built-in function. We return
 *  true if the argument is a boolean, and false otherwise.
\*/

void setl2_is_boolean(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   if (argv[0].sp_form == ft_atom &&
       (argv[0].sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num ||
        argv[0].sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num)) {

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
}

/*\
 *  \function{setl2\_is\_integer()}
 *
 *  This function is the \verb"is_integer" built-in function. We return
 *  true if the argument is an integer, and false otherwise.
\*/

void setl2_is_integer(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   if (argv[0].sp_form == ft_short || argv[0].sp_form == ft_long) {

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
}

/*\
 *  \function{setl2\_is\_real()}
 *
 *  This function is the \verb"is_real" built-in function. We return
 *  true if the argument is a real, and false otherwise.
\*/

void setl2_is_real(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   if (argv[0].sp_form == ft_real) {

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
}

/*\
 *  \function{setl2\_is\_string()}
 *
 *  This function is the \verb"is_string" built-in function. We return
 *  true if the argument is a string, and false otherwise.
\*/

void setl2_is_string(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   if (argv[0].sp_form == ft_string) {

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
}

/*\
 *  \function{setl2\_is\_set()}
 *
 *  This function is the \verb"is_set" built-in function. We return
 *  true if the argument is a set, and false otherwise.
\*/

void setl2_is_set(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   if (argv[0].sp_form == ft_set || argv[0].sp_form == ft_map) {

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
}

/*\
 *  \function{setl2\_is\_map()}
 *
 *  This function is the \verb"is_map" built-in function. We return
 *  true if the argument is a map, and false otherwise.
\*/

void setl2_is_map(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   if (argv[0].sp_form == ft_map) {

      unmark_specifier(target);
      target->sp_form = ft_atom;
      target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

      return;

   }

   if (argv[0].sp_form == ft_set && set_to_map(SETL_SYSTEM argv,argv,NO)) {

      unmark_specifier(target);
      target->sp_form = ft_atom;
      target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

      return;

   }

   unmark_specifier(target);
   target->sp_form = ft_atom;
   target->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

   return;

}

/*\
 *  \function{setl2\_is\_tuple()}
 *
 *  This function is the \verb"is_tuple" built-in function. We return
 *  true if the argument is a tuple, and false otherwise.
\*/

void setl2_is_tuple(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   if (argv[0].sp_form == ft_tuple) {

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
}

/*\
 *  \function{setl2\_is\_procedure()}
 *
 *  This function is the \verb"is_procedure" built-in function. We return
 *  true if the argument is a procedure, and false otherwise.
\*/

void setl2_is_procedure(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   if (argv[0].sp_form == ft_proc) {

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
} 
