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
 *  \package{System Procedures}
 *
 *  We provide a access to a few system services in this file.  We are
 *  trying to provide the minimum necessary, since this stuff is
 *  non-portable.
\*/

#ifdef MACINTOSH
#include <Files.h>
#include <PPCToolBox.h>
#include <CodeFragments.h>
#endif /* MACINTOSH */

#ifdef HAVE_GETRUSAGE
#include <sys/time.h>
#include <sys/resource.h>
#endif


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "builtins.h"                  /* built-in symbols                  */
#include "libman.h"                    /* library manager                   */
#include "unittab.h"                   /* unit table                        */
#include "loadunit.h"                  /* unit loader                       */
#include "x_integers.h"                /* integers                          */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "tuples.h"                    /* tuples                            */
#include "x_reals.h"                   /* real numbers                      */
#include "x_strngs.h"                  /* strings                           */

#ifdef PLUGIN
#define fprintf plugin_fprintf
char *JavaScript(char *js);
extern char *javascript_buffer;
extern int javascript_buffer_len;
#endif

#ifdef DYNAMIC_COMP
int compile_fragment(SETL_SYSTEM_PROTO char *program_source, int compile_flag);
extern long numeval;
extern int defining_proc;
extern global_ptr_type global_head;
#endif

#ifdef WIN32
#include <windows.h>
#include <stdlib.h>
#include "axobj.h"
#endif


/*\
 *  \function{setl2\_exists()}
 *
 *  This function tests for existence of a file.  It does not check
 *  permissions.
\*/

void setl2_fexists(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
char file_name[PATH_LENGTH + 1];       /* file name                         */
char *s, *t;                           /* temporary looping variables       */

   /* convert the file name to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"fexists",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   if (string_hdr->s_length > PATH_LENGTH)
      abend(SETL_SYSTEM msg_bad_file_spec,abend_opnd_str(SETL_SYSTEM argv));

   t = file_name;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < file_name + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   /* return True or False as appropriate */

   unmark_specifier(target);
   if (os_access(file_name,0) == 0) {

      target->sp_form = ft_atom;
      target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

   }
   else {

      target->sp_form = ft_atom;
      target->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

   }

   return;

}

/*\
 *  \function{setl2\_system()}
 *
 *  This procedure calls the operating system, passing it a command.  It
 *  does no error checking of that command.
\*/

void setl2_system(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
char *command;                         /* system command                    */
char *s, *t;                           /* temporary looping variables       */

   /* convert the command to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"system",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   command = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (command == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = command;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < command + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   /* execute the command, and free the string */

   system(command);

   free(command);

   /* return om */

   unmark_specifier(target);
   target->sp_form = ft_omega;
   return;

}

/*\
 *  \function{setl2\_abort()}
 *
 *  This procedure gives the \setl\ programmer access to the abend
 *  function.
\*/

void setl2_abort(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
char *message;                         /* error message                     */
char *s, *t;                           /* temporary looping variables       */

   /* convert the message to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"abort",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   message = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (message == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = message;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < message + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   /* abort, with message */

   abend(SETL_SYSTEM message);

}

/*\
 *  \function{setl2\_trace()}
 *
 *  This function turns tracing on and off during debugging.
\*/


void setl2_trace(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{

#ifdef DEBUG
   /* the argument must be true or false */

   if (argv[0].sp_form != ft_atom ||
       (argv[0].sp_val.sp_atom_num != spec_true->sp_val.sp_atom_num &&
        argv[0].sp_val.sp_atom_num != spec_false->sp_val.sp_atom_num))
      abend(SETL_SYSTEM msg_bad_arg,"boolean",1,"trace",
            abend_opnd_str(SETL_SYSTEM argv));

   if (argv[0].sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num) {

      fprintf(DEBUG_FILE,"\nTracing ON\n\nSETL2 source file => %s\n\n",
                         X_SOURCE_NAME);
      TRACING_ON = YES;

   }
   else {

      fprintf(DEBUG_FILE,"\nTracing OFF\n");
      TRACING_ON = NO;

   }
#endif
   /* return om */

   unmark_specifier(target);
   target->sp_form = ft_omega;
   return;

}

/*\
 *  \function{setl2\_ref\_count()}
 *
 *  This procedure prints to debug_file the reference count of an object
\*/

void setl2_ref_count(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
int x;


#ifdef DEBUG
   if ((argv[0].sp_form <ft_proc)&&(argv[0].sp_form !=ft_opaque))
      fprintf(DEBUG_FILE,"Not a set or map\n");
   else {
      x=(int)(*((int32 *)(argv[0].sp_val.sp_biggest)))-1;;
      fprintf(DEBUG_FILE,"Type %d:(%s) Reference count : %d\n",argv[0].sp_form,
         abend_opnd_str(SETL_SYSTEM &argv[0]),x);
   }

   /* return om */
#endif

   unmark_specifier(target);
   target->sp_form = ft_omega;
   return;
}


/*\
 *  \function{setl2\_opcode\_count()}
 *
 *  This procedure returns the number of pseudo-opcodes executed so far.
\*/

void setl2_opcode_count(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
int32 short_hi_bits;                   /* high order bits of short          */

   /* check whether the size is short */

   if (!(short_hi_bits = OPCODE_COUNT & INT_HIGH_BITS) ||
       short_hi_bits == INT_HIGH_BITS) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = OPCODE_COUNT;

   }
   else {
  
      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,OPCODE_COUNT);

   }

   return;

}

/*\
 *  \function{setl2\_getenv()}
 *
 *  This function returns the value of an environment variable.
\*/

void setl2_getenv(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *key;                             /* system key                        */
char *s, *t;                           /* temporary looping variables       */

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"getenv",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   key = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = key;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < key + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   /* check whether there is an environment string with this key */

   s = getenv(key);
   free(key);

   if (s == NULL) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   /* first we make a SETL2 string out of the C string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = NULL;
   string_char_ptr = string_char_end = NULL;

   /* copy the source string */

   while (*s) {

      if (string_char_ptr == string_char_end) {

         get_string_cell(string_cell);
         if (string_hdr->s_tail != NULL)
            (string_hdr->s_tail)->s_next = string_cell;
         string_cell->s_prev = string_hdr->s_tail;
         string_cell->s_next = NULL;
         string_hdr->s_tail = string_cell;
         if (string_hdr->s_head == NULL)
            string_hdr->s_head = string_cell;
         string_char_ptr = string_cell->s_cell_value;
         string_char_end = string_char_ptr + STR_CELL_WIDTH;

      }

      *string_char_ptr++ = *s++;
      string_hdr->s_length++;

   }

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;

}

/*\
 *  \function{setl2\_user_time()}
 *
 *  This function returns the amount of CPU time used, in seconds.
\*/

void setl2_user_time(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
i_real_ptr_type real_ptr;              /* real pointer                      */
double real_number;                    /* real value                        */
	real_number = 0;

#ifndef WIN32
#ifdef HAVE_GETRUSAGE 
struct timeval usage;                   /* structure to fill                 */

   /* call the C library function and check for an error */

   getrusage(RUSAGE_SELF, &usage);
   real_number = (double)usage.tv_sec +
                 (double)usage.tv_usec / 1000000.0;
   
#endif
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
 *  \function{setl2\_library\_file()}
 *
 *  This function returns a file which is stored in the library.
\*/

void setl2_library_file(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *key;                             /* system key                        */
char *s, *t;                           /* temporary looping variables       */
libunit_ptr_type libunit_ptr;          /* work library unit pointer         */
libstr_ptr_type textstr_ptr, lenstr_ptr;
                                       /* text stream and length stream     */
libstr_ptr_type libstr_ptr;            /* unit control stream               */
unit_control_record unit_control;      /* unit control record               */
int32 length;                          /* length of current record          */
tuple_h_ptr_type tuple_root, tuple_work_hdr, new_tuple_hdr;
                                       /* tuple header pointers             */
tuple_c_ptr_type tuple_cell;           /* tuple cell pointer                */
int tuple_index, tuple_height;         /* used to descend header trees      */
int32 tuple_length;                    /* current tuple length              */
int32 expansion_trigger;               /* size which triggers header        */
                                       /* expansion                         */
string_h_ptr_type target_hdr;          /* target string                     */
string_c_ptr_type target_cell;         /* target string cell                */
int32 i, j, count;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"library_file",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   key = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = key;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < key + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   /*
    *  Find the unit in the library list.
    */

   libunit_ptr = open_libunit(SETL_SYSTEM key, NULL, LIB_READ_UNIT);
   free(key);
   if (libunit_ptr == NULL) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   /* load the unit control record */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr, LIB_CONTROL_STREAM);
   read_libstr(SETL_SYSTEM libstr_ptr, (char *)&unit_control,
               sizeof(unit_control_record));
   close_libstr(SETL_SYSTEM libstr_ptr);

   if (unit_control.uc_type != FILE_UNIT) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   textstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr, LIB_TEXT_STREAM);
   lenstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr, LIB_LENGTH_STREAM);

   /*
    *  We have to initialize a tuple to hold the lines we will read.
    */

   get_tuple_header(tuple_root);
   tuple_root->t_use_count = 1;
   tuple_root->t_hash_code = 0;
   tuple_root->t_ntype.t_root.t_length = 0;
   tuple_root->t_ntype.t_root.t_height = 0;
   for (i = 0;
        i < TUP_HEADER_SIZE;
        tuple_root->t_child[i++].t_cell = NULL);
   tuple_length = 0;
   expansion_trigger = TUP_HEADER_SIZE;

   /* loop through the lines of the file */

   for (count = 0; count < unit_control.uc_line_count; count++) {
      
      read_libstr(SETL_SYSTEM lenstr_ptr, (char *)&length, sizeof(int32));

      /* make a target string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = length;
      target_hdr->s_head = target_hdr->s_tail = NULL;

      /* copy the argument to the string */

      while (length > 0) {

         get_string_cell(target_cell);
         if (target_hdr->s_tail != NULL)
            (target_hdr->s_tail)->s_next = target_cell;
         target_cell->s_prev = target_hdr->s_tail;
         target_cell->s_next = NULL;
         target_hdr->s_tail = target_cell;
         if (target_hdr->s_head == NULL)
            target_hdr->s_head = target_cell;

         j = min(length, STR_CELL_WIDTH);
         read_libstr(SETL_SYSTEM textstr_ptr, target_cell->s_cell_value, j);
         length -= j;

      }

      /*
       *  Now we have to insert the string we just created into the
       *  return tuple.
       */

      /* expand the tuple tree if necessary */

      if (tuple_length >= expansion_trigger) {

         tuple_work_hdr = tuple_root;

         get_tuple_header(tuple_root);

         tuple_root->t_use_count = 1;
         tuple_root->t_hash_code =
            tuple_work_hdr->t_hash_code;
         tuple_root->t_ntype.t_root.t_length =
            tuple_work_hdr->t_ntype.t_root.t_length;
         tuple_root->t_ntype.t_root.t_height =
            tuple_work_hdr->t_ntype.t_root.t_height + 1;

         for (i = 1;
              i < TUP_HEADER_SIZE;
              tuple_root->t_child[i++].t_header = NULL);

         tuple_root->t_child[0].t_header = tuple_work_hdr;

         tuple_work_hdr->t_ntype.t_intern.t_parent = tuple_root;
         tuple_work_hdr->t_ntype.t_intern.t_child_index = 0;

         expansion_trigger *= TUP_HEADER_SIZE;

      }

      tuple_root->t_ntype.t_root.t_length++;

      /* descend the tree to a leaf */

      tuple_work_hdr = tuple_root;
      for (tuple_height = tuple_work_hdr->t_ntype.t_root.t_height;
           tuple_height;
           tuple_height--) {

         /* extract the element's index at this level */

         tuple_index = (tuple_length >>
                              (tuple_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (tuple_work_hdr->t_child[tuple_index].t_header == NULL) {

            get_tuple_header(new_tuple_hdr);
            new_tuple_hdr->t_ntype.t_intern.t_parent = tuple_work_hdr;
            new_tuple_hdr->t_ntype.t_intern.t_child_index = tuple_index;
            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_tuple_hdr->t_child[i++].t_cell = NULL);
            tuple_work_hdr->t_child[tuple_index].t_header =
                  new_tuple_hdr;
            tuple_work_hdr = new_tuple_hdr;

         }
         else {

            tuple_work_hdr =
               tuple_work_hdr->t_child[tuple_index].t_header;

         }
      }

      /*
       *  At this point, tuple_work_hdr points to the lowest level header
       *  record.  We insert the new element.
       */

      tuple_index = tuple_length & TUP_SHIFT_MASK;
      get_tuple_cell(tuple_cell);
      tuple_cell->t_spec.sp_form = ft_string;
      tuple_cell->t_spec.sp_val.sp_string_ptr = target_hdr;
      spec_hash_code(tuple_cell->t_hash_code,&(tuple_cell->t_spec));
      tuple_root->t_hash_code ^= tuple_cell->t_hash_code;
      tuple_work_hdr->t_child[tuple_index].t_cell = tuple_cell;

      /* increment the tuple size */

      tuple_length++;

   }
   tuple_root->t_ntype.t_root.t_length = tuple_length;

   /* we're done with the library */

   close_libstr(SETL_SYSTEM textstr_ptr);
   close_libstr(SETL_SYSTEM lenstr_ptr);
   close_libunit(SETL_SYSTEM libunit_ptr);

   /* set the return value to the tuple we just created */

   unmark_specifier(target);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = tuple_root;

   return;

}

/*\
 *  \function{setl2\_library\_package()}
 *
 *  This function returns a package which is stored in the library.
\*/

void setl2_library_package(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *key;                             /* system key                        */
char *s, *t;                           /* temporary looping variables       */
unittab_ptr_type unittab_ptr;          /* loaded unit                       */

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg, "string", 1, "library_package",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   key = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = key;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < key + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   unittab_ptr = load_unit(SETL_SYSTEM key, NULL, NULL);
   free(key);

   /*
    *  Make sure we found a package.
    */

   if (unittab_ptr == NULL || 
       ((unittab_ptr->ut_type != PACKAGE_UNIT)&&
       (unittab_ptr->ut_type != NATIVE_UNIT))) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   /*
    *  Return the symbol map.
    */

   unmark_specifier(target);
   target->sp_form = ft_map;
   target->sp_val.sp_map_ptr = unittab_ptr->ut_symbol_map;
   mark_specifier(target);

   return;

}

/*\
 *  \function{setl2\_eval()}
 *
 *  This function dynamically loads a program.
\*/

void setl2_eval(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
#ifdef DYNAMIC_COMP
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *key;                             /* system key                        */
char *s, *t;                           /* temporary looping variables       */
unittab_ptr_type unit_ptr;  
struct instruction_item *pc_old;      
char *fragment;                        /* fragment to compile               */
int compile_result;
char eval_name[32];
global_ptr_type temp_global_head;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"eval",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   key = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = key;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < key + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   
   fragment = (char *)malloc((size_t)(strlen(key)+200));
   if (fragment == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   if (EVAL_PACKAGE==NO) {
             sprintf(fragment,"package eval_vars; var eval_0000; end eval_vars; program eval_prog%ld; use eval_vars;\n%s\n end eval_prog%ld;\n",
                     NUMEVAL,key,NUMEVAL);
	     EVAL_PACKAGE=YES;
   } else {
             sprintf(fragment,"program eval_prog%ld; use eval_vars;\n%s\n end eval_prog%ld;\n",NUMEVAL,key,NUMEVAL);
   }
   free(key);

   compile_result=compile_fragment(SETL_SYSTEM fragment,YES);  /* Compiling a fragment */ 
   free(fragment);
   if (compile_result!=SUCCESS_EXIT) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   sprintf(eval_name,"EVAL_PROG%ld",NUMEVAL);
   if (DEFINING_PROC)
      NUMEVAL++;
   unit_ptr = load_eval_unit(SETL_SYSTEM eval_name, NULL, NULL);
   pc_old=pc;
   execute_setup(SETL_SYSTEM unit_ptr, EX_BODY_CODE); 
   execute_go(SETL_SYSTEM YES);
   pc=pc_old;
   if (DEFINING_PROC) {
      temp_global_head = GLOBAL_HEAD;
      while ( temp_global_head!=NULL ) {
        if (temp_global_head->gl_offset>=0) {
          unit_ptr->ut_unit_tab[2]->ut_data_ptr[temp_global_head->gl_number]=unit_ptr->ut_unit_tab[1]->ut_data_ptr[temp_global_head->gl_offset];

          temp_global_head->gl_offset=0;
          temp_global_head->gl_type=0;
         }
         temp_global_head=temp_global_head->gl_next_ptr;
      }
   }


   /* Return 0, meaning eval() succeded ... */

   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value = 0;

#endif 
}

/*\
 *
\*/

void setl2_javascript(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *key;                             /* system key                        */
char *s, *t;                           /* temporary looping variables       */
unittab_ptr_type unittab_ptr;          /* loaded unit                       */
int ljs;			       /* Total length of the command       */

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg, "string", 1, "javascript",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;
 
   ljs=string_hdr->s_length +1;

   key = (char *)malloc((size_t)(ljs));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = key;

   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < key + ljs-1 &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

#ifdef PLUGIN
   javascript_buffer=NULL;
   JavaScript(key);
   s=javascript_buffer;
   if (s==NULL) {
        free(key);
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }
#else 
   s="";
#endif
   free(key);

   /* first we make a SETL2 string out of the C string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = NULL;
   string_char_ptr = string_char_end = NULL;

   /* copy the source string */

   while (*s) {

      if (string_char_ptr == string_char_end) {

         get_string_cell(string_cell);
         if (string_hdr->s_tail != NULL)
            (string_hdr->s_tail)->s_next = string_cell;
         string_cell->s_prev = string_hdr->s_tail;
         string_cell->s_next = NULL;
         string_hdr->s_tail = string_cell;
         if (string_hdr->s_head == NULL)
            string_hdr->s_head = string_cell;
         string_char_ptr = string_cell->s_cell_value;
         string_char_end = string_char_ptr + STR_CELL_WIDTH;

      }

      *string_char_ptr++ = *s++;
      string_hdr->s_length++;

   }
#ifdef PLUGIN
  if (javascript_buffer!=NULL) free(javascript_buffer);
#endif 

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;

}

void setl2_wait(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *key;                             /* system key                        */
char *s, *t;                           /* temporary looping variables       */
unittab_ptr_type unittab_ptr;          /* loaded unit                       */
int ljs;			       /* Total length of the command       */

/* wait is only valid for >0 */

   if (argv[0].sp_form != ft_short ||
       argv[0].sp_val.sp_short_value < 1 ||
       argv[0].sp_val.sp_short_value > 256)
      abend(SETL_SYSTEM msg_bad_arg,"integer",1,"wait",
            abend_opnd_str(SETL_SYSTEM argv));
#ifdef PLUGIN
	WAIT_FLAG=-argv[0].sp_val.sp_short_value;
#endif

   unmark_specifier(target);
   target->sp_form = ft_omega;
   return;

}

void setl2_pass_symtab(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *key;                             /* system key                        */
char *s, *t;                           /* temporary looping variables       */
unittab_ptr_type unittab_ptr;          /* loaded unit                       */


   if (argv[0].sp_form != ft_set)
      abend(SETL_SYSTEM msg_bad_arg,"set",1,"pass_symtab",
            abend_opnd_str(SETL_SYSTEM argv));

   if (!set_to_map(SETL_SYSTEM &argv[0],&argv[0],NO))
            abend(SETL_SYSTEM msg_invalid_set_map,
                  abend_opnd_str(SETL_SYSTEM &argv[0]));


   unmark_specifier(&SYMBOL_MAP);
   SYMBOL_MAP.sp_form = ft_map;
   SYMBOL_MAP.sp_val.sp_map_ptr = argv[0].sp_val.sp_map_ptr;
   mark_specifier(&SYMBOL_MAP);

   return;

}

void setl2_geturl(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
#ifndef PLUGIN
int javascript_buffer_len = 0;
#endif

string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *key;                             /* system key                        */
char *s, *t;                           /* temporary looping variables       */
unittab_ptr_type unittab_ptr;          /* loaded unit                       */
int ljs;			       /* Total length of the command       */

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg, "string", 1, "geturl",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;
 
   ljs=string_hdr->s_length +1;

   key = (char *)malloc((size_t)(ljs));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = key;

   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < key + ljs-1 &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';


#ifdef PLUGIN
   javascript_buffer=NULL;
   GetUrl(key);
   s=javascript_buffer;
   if (s==NULL) {
   	  free(key);
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }
#else
   s="";
#endif
   free(key);

   /* first we make a SETL2 string out of the C string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = NULL;
   string_char_ptr = string_char_end = NULL;

   /* copy the source string */

   while (--javascript_buffer_len>0) {


      if (string_char_ptr == string_char_end) {

         get_string_cell(string_cell);
         if (string_hdr->s_tail != NULL)
            (string_hdr->s_tail)->s_next = string_cell;
         string_cell->s_prev = string_hdr->s_tail;
         string_cell->s_next = NULL;
         string_hdr->s_tail = string_cell;
         if (string_hdr->s_head == NULL)
            string_hdr->s_head = string_cell;
         string_char_ptr = string_cell->s_cell_value;
         string_char_end = string_char_ptr + STR_CELL_WIDTH;

      }

      *string_char_ptr++ = *s++;
      string_hdr->s_length++;

   }
#ifdef PLUGIN
  if (javascript_buffer!=NULL) free(javascript_buffer);
#endif

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;

}



void setl2_posturl(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
#ifndef PLUGIN
int javascript_buffer_len = 0;
#endif

string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *key;                             /* system key                        */
char *postdata;
char *s, *t;                           /* temporary looping variables       */
unittab_ptr_type unittab_ptr;          /* loaded unit                       */
int ljs;			       /* Total length of the command       */

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg, "string", 1, "posturl",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;
 
   ljs=string_hdr->s_length +1;

   key = (char *)malloc((size_t)(ljs));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = key;

   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < key + ljs-1 &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';
   
    if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg, "string", 2, "posturl",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[1].sp_val.sp_string_ptr;
 
   ljs=string_hdr->s_length +1;

   postdata = (char *)malloc((size_t)(ljs));
   if (postdata == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = postdata;

   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < postdata + ljs-1 &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

#ifdef PLUGIN
   javascript_buffer=NULL;
   PostUrl(key,postdata);
   s=javascript_buffer;
   if (s==NULL) {
   	  free(key);
   	  free(postdata);
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }
#else
   s="";
#endif
   free(key);
   free(postdata);

   /* first we make a SETL2 string out of the C string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = NULL;
   string_char_ptr = string_char_end = NULL;

   /* copy the source string */

   while (--javascript_buffer_len>0) {


      if (string_char_ptr == string_char_end) {

         get_string_cell(string_cell);
         if (string_hdr->s_tail != NULL)
            (string_hdr->s_tail)->s_next = string_cell;
         string_cell->s_prev = string_hdr->s_tail;
         string_cell->s_next = NULL;
         string_hdr->s_tail = string_cell;
         if (string_hdr->s_head == NULL)
            string_hdr->s_head = string_cell;
         string_char_ptr = string_cell->s_cell_value;
         string_char_end = string_char_ptr + STR_CELL_WIDTH;

      }

      *string_char_ptr++ = *s++;
      string_hdr->s_length++;

   }
#ifdef PLUGIN
  if (javascript_buffer!=NULL) free(javascript_buffer);
#endif

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;

}


void setl2_create_activexobject(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
#ifdef SPAM

HRESULT hr;
LPDISPATCH lpDispatch;   
CLSID    clsid;
wchar_t* wName;
int cLen;
struct setl_ax * A; 

string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *key;                             /* system key                        */
char *s, *t;                           /* temporary looping variables       */
unittab_ptr_type unittab_ptr;          /* loaded unit                       */
int ljs;			       /* Total length of the command       */

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg, "string", 1, "createactivexobject",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;
 
   ljs=string_hdr->s_length +1;

   key = (char *)malloc((size_t)(ljs));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = key;

   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < key + ljs-1 &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';



	cLen = mbstowcs(0,key,strlen(key)+1);
    wName = (wchar_t*)malloc((cLen+1)*sizeof(wchar_t));
    mbstowcs(wName,key,strlen(key)+1);
	free(key);

    if (CLSIDFromProgID(wName,&clsid)!=S_OK)  {
		  unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
	}

	free(wName);

	hr=CoCreateInstance(&clsid,NULL,CLSCTX_ALL,&IID_IDispatch,(void **)&lpDispatch);

    if(!FAILED(hr)) {
		A = (struct setl_ax *)(malloc(sizeof(struct setl_ax)));
		if (A == NULL)
			giveup(SETL_SYSTEM msg_malloc_error);

	    A->use_count = 1;
		A->type = ax_type;

		A->lpDispatch=lpDispatch;
   
		unmark_specifier(target);
		target->sp_form = ft_opaque;
		target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
		return;
	}

#endif


   unmark_specifier(target);
   target->sp_form = ft_omega;
   return;



}

