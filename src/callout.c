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
 *  \package{Interim Callout Facility}
 *
 *  This package provides an extremely crude, but functional, callout
 *  facility for \setl.  All calls to C will be through the function
 *  \verb"setl2_Ccallout", and will call the function
 *  \verb"setl2_callout", which the user must provide.  Calls to \setl\
 *  will be through the procedure passed to \verb"callout"
\*/


/* standard C header files */

#if UNIX_VARARGS
#include <varargs.h>                   /* variable argument macros          */
#else
#include <stdarg.h>                    /* variable argument macros          */
#endif

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "builtins.h"                  /* built-in symbols                  */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_strngs.h"                  /* strings                           */
#include "tuples.h"                    /* tuples                            */
#include "procs.h"                     /* procedures                        */
#include "x_reals.h"


#ifdef MACINTOSH
#include <Types.h>
#include <Files.h>
#include <PPCToolBox.h>
#include <CodeFragments.h>
#include <TextUtils.h>
#include <ctype.h>
#endif /* MACINTOSH */

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

/* intermediate file ast record */

struct comm_block {
   int32 cb_length;                    /* communication block length        */
   int32 cb_ptr_length;                /* size of arg vector                */
   char **cb_ptr;                      /* arg vector buffer                 */
   int32 cb_str_length;                /* size of string buffer             */
   char *cb_str;                       /* string buffer                     */
   struct comm_block *cb_next;         /* next block in stack               */
};

/* return structure for Jack's version */

struct return_struct {
   long rs_length;
   char *rs_data;
};

/* local data */

static specifier callback;             /* saved callback handler            */
static specifier callback2;            /* saved callback handler            */
static struct comm_block *next_comm_block = NULL;
                                       /* next communication block to be    */
                                       /* used                              */
static struct comm_block **comm_list_tail = &next_comm_block;
                                       /* tail of comm block list           */

/* this must be provided by the user */

extern char *setl2_callout(SETL_SYSTEM_PROTO int, int, char **);
extern struct return_struct *setl2_callout2(SETL_SYSTEM_PROTO int, int, char **);

/* additional functions */
void setl2_reset_callback(SETL_SYSTEM_PROTO_VOID);

/*\
 *  \function{setl2\_Ccallout()}
 *
 *  This is the entry function for callout.  The user passes an integer
 *  service code, a callback handler, and a tuple of arguments for C.  We
 *  convert these arguments to C form and pass them to a user-provided
 *  function.
\*/

void setl2_Ccallout(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (3 here)          */
   specifier *target)                  /* return value                      */

{
int function_type;                     /* output function code              */
int count;                             /* number of arguments               */
int total_length;                      /* total string length               */
struct comm_block *save_comm_block;    /* block used by this instance       */
char *arg_string;                      /* argument storage string           */
char **arg_vector;                     /* callout argument vector           */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* tuple element                     */
string_h_ptr_type string_hdr;
                                       /* source and target strings         */
string_c_ptr_type string_cell;
                                       /* source and target string cells    */
int32 string_length;                   /* source string length              */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *p;                               /* temporary looping variable        */

   /* the first argument must be an integer */

   if (argv[0].sp_form == ft_short) {

      function_type = (int)(argv[0].sp_val.sp_short_value);

   }
   else if (argv[0].sp_form == ft_long) {

      function_type = (int)long_to_short(SETL_SYSTEM argv[0].sp_val.sp_long_ptr);

   }
   else {

      abend(SETL_SYSTEM msg_bad_arg,"integer",1,"callout",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* the second must be the callback handler (don't check now) */

   callback.sp_form = argv[1].sp_form;
   callback.sp_val.sp_biggest = argv[1].sp_val.sp_biggest;
   mark_specifier(&callback);

   /* the last must be a tuple of strings */

   if (argv[2].sp_form != ft_tuple)
      abend(SETL_SYSTEM msg_bad_arg,"tuple",3,"callout",
            abend_opnd_str(SETL_SYSTEM argv+2));

   /* set up to loop over the tuple, counting arguments and lengths */

   count = 0;
   total_length = 0;
   source_root = argv[2].sp_val.sp_tuple_ptr;
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

      /* we expect each element of the tuple to be a string */

      if (count < source_number)
         abend(SETL_SYSTEM msg_bad_arg,"tuple of strings",3,"callout",
               abend_opnd_str(SETL_SYSTEM argv+2));

      if (source_element->sp_form != ft_string)
         abend(SETL_SYSTEM msg_bad_arg,"tuple of strings",3,"callout",
               abend_opnd_str(SETL_SYSTEM argv+2));

      count++;
      total_length += (source_element->sp_val.sp_string_ptr)->s_length + 1;

   }

   /* we've calculated how much space we need, now we allocate it */

   if (next_comm_block == NULL) {

      next_comm_block = 
         (struct comm_block *)malloc(sizeof(struct comm_block));
      if (next_comm_block == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);

      comm_list_tail = &(next_comm_block->cb_next);
      next_comm_block->cb_ptr_length = 0;
      next_comm_block->cb_ptr = NULL;
      next_comm_block->cb_str_length = 0;
      next_comm_block->cb_str = NULL;
      next_comm_block->cb_next = NULL;

   }     

   if (next_comm_block->cb_ptr_length < count) {

      if (next_comm_block->cb_ptr != NULL) 
         free(next_comm_block->cb_ptr);
      next_comm_block->cb_ptr =
         (char **)malloc((size_t)(count * sizeof(char *)));
      if (next_comm_block->cb_ptr == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      next_comm_block->cb_ptr_length = count;

   }

   if (next_comm_block->cb_str_length < total_length) {

      if (next_comm_block->cb_str != NULL) 
         free(next_comm_block->cb_str);
      next_comm_block->cb_str =
         (char *)malloc((size_t)total_length);
      if (next_comm_block->cb_str == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      next_comm_block->cb_str_length = total_length;

   }

   arg_vector = next_comm_block->cb_ptr;
   arg_string = next_comm_block->cb_str;
   save_comm_block = next_comm_block;
   next_comm_block = next_comm_block->cb_next;

   /* loop over the tuple again, building up the array for C */

   p = arg_string;
   count = 0;

   source_root = argv[2].sp_val.sp_tuple_ptr;
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

      /* we have an element, copy the string into our buffer */

      arg_vector[count] = p;
      count++;

      /* initialize the source string */

      string_hdr = source_element->sp_val.sp_string_ptr;
      string_length = string_hdr->s_length;
      string_cell = string_hdr->s_head;
      if (string_cell == NULL) {
         string_char_ptr = string_char_end = NULL;
      }
      else {
         string_char_ptr = string_cell->s_cell_value;
         string_char_end = string_char_ptr + STR_CELL_WIDTH;
      }

      /* copy the string into the buffer */

      while (string_length--) {

         if (string_char_ptr == string_char_end) {

            string_cell = string_cell->s_next;
            string_char_ptr = string_cell->s_cell_value;
            string_char_end = string_char_ptr + STR_CELL_WIDTH;

         }

         *p++ = *string_char_ptr++;

      }

      *p++ = '\0';

   }

   /* finally, we're ready to call the C version of this function */

   p = setl2_callout(SETL_SYSTEM function_type,count,arg_vector);

   /* we have to create a return value for SETL2 */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = NULL;
   string_char_ptr = string_char_end = NULL;

   /* copy the return string */

   if (p != NULL) {

      while (*p) {

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

         *string_char_ptr++ = *p++;
         string_hdr->s_length++;

      }
   }

   /* set the return value */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   /* free our string buffers */

   next_comm_block = save_comm_block;
   unmark_specifier(&callback);

   return;

}

/*\
 *  \function{setl2\_callback()}
 *
 *  This function handles callbacks from C.  We convert the arguments
 *  from C to \setl\ form, then execute the \setl\ procedure.
\*/

#if UNIX_VARARGS
char *setl2_callback(va_alist)
   va_dcl                              /* all arguments                     */
#else
char *setl2_callback(
   SETL_SYSTEM_PROTO
   char *firstarg,                     /* first argument from caller        */
   ...)                                /* any other arguments               */
#endif

{
va_list argp;                          /* current argument pointer          */
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
int32 string_length;                   /* source string length              */
tuple_h_ptr_type tuple_root, tuple_work_hdr, new_tuple_hdr;
                                       /* tuple header pointers             */
tuple_c_ptr_type tuple_cell;           /* tuple cell pointer                */
int32 tuple_length,tuple_height,tuple_index;
                                       /* used to create return tuple       */
static char *return_string = NULL;     /* static return buffer              */
int32 expansion_trigger,work_hash_code;
                                       /* used to build tuple               */
int first_arg;                         /* YES if first argument             */
specifier spare;                       /* spare specifier                   */
specifier save_callback;               /* saved callback handler            */
char *p;                               /* temporary looping varible         */
int i;                                 /* temporary looping variable        */

   /* make sure our callback is on */

   if (callback.sp_form == ft_void)
		return ""; 
		
   /* make sure our callback is a procedure */

   if (callback.sp_form != ft_proc)
      abend(SETL_SYSTEM "Expected procedure in callout, but found:\n %s",
            abend_opnd_str(SETL_SYSTEM &callback));

   /* create an empty tuple */

   get_tuple_header(tuple_root);
   tuple_root->t_use_count = 1;
   tuple_root->t_hash_code = 0;
   tuple_root->t_ntype.t_root.t_length = 0;
   tuple_root->t_ntype.t_root.t_height = 0;

   for (i = 0;
        i < TUP_HEADER_SIZE;
        tuple_root->t_child[i++].t_cell = NULL);

   /* insert each element in the tuple */

   tuple_length = 0;
   first_arg = YES;
#if UNIX_VARARGS
   va_start(argp);
   p = va_arg(argp, char *);
#else
   va_start(argp,firstarg);
   p = firstarg;
#endif
   while (p != NULL) {

      /* first we make a SETL2 string out of the C string */

      get_string_header(string_hdr);
      string_hdr->s_use_count = 1;
      string_hdr->s_hash_code = -1;
      string_hdr->s_length = 0;
      string_hdr->s_head = string_hdr->s_tail = NULL;
      string_char_ptr = string_char_end = NULL;

      /* copy the source string */

      while (*p) {

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

         *string_char_ptr++ = *p++;
         string_hdr->s_length++;

      }

      /*
       *  We've converted one of the C arguments into a SETL2 string.
       *  We push the first argument, and append others to our tuple.
       */

      if (first_arg) {

         first_arg = NO;
         spare.sp_form = ft_string;
         spare.sp_val.sp_string_ptr = string_hdr;
         push_pstack(&spare);
         string_hdr->s_use_count--;

         p = va_arg(argp, char *);
         continue;

      }

      /* add it to the tuple */

      expansion_trigger = 1 << ((tuple_root->t_ntype.t_root.t_height + 1)
                                 * TUP_SHIFT_DIST);

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

      }

      tuple_root->t_ntype.t_root.t_length = tuple_length + 1;

      /* find the tuple component */

      tuple_work_hdr = tuple_root;
      for (tuple_height = tuple_root->t_ntype.t_root.t_height;
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
            new_tuple_hdr->t_ntype.t_intern.t_child_index = (int)tuple_index;
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
      tuple_work_hdr->t_child[tuple_index].t_cell = tuple_cell;
      tuple_cell->t_spec.sp_form = ft_string;
      tuple_cell->t_spec.sp_val.sp_string_ptr = string_hdr;
      spec_hash_code(work_hash_code,&(tuple_cell->t_spec));
      tuple_root->t_hash_code ^= work_hash_code;
      tuple_cell->t_hash_code = work_hash_code;

      /* get the next argument */

      tuple_length++;
      p = va_arg(argp, char *);

   }

   /* if we didn't get any arguments, push an omega */

   if (first_arg) {

      spare.sp_form = ft_omega;
      push_pstack(&spare);

   }

   /* push our tuple */

   spare.sp_form = ft_tuple;
   spare.sp_val.sp_tuple_ptr = tuple_root;
   push_pstack(&spare);
   tuple_root->t_use_count--;

   /* call the callback handler */

   save_callback.sp_form = callback.sp_form;
   save_callback.sp_val.sp_biggest = callback.sp_val.sp_biggest;
   spare.sp_form = ft_omega;
   call_procedure(SETL_SYSTEM &spare,
                  &save_callback,
                  NULL,
                  2L,YES,NO,0);
   callback.sp_form = save_callback.sp_form;
   callback.sp_val.sp_biggest = save_callback.sp_val.sp_biggest;

   /* release any old return string */

   if (return_string != NULL) {
      free(return_string);
      return_string = NULL;
   }

   /* build up the C string */

   if (spare.sp_form == ft_string) {

      /* initialize the source string */

      string_hdr = spare.sp_val.sp_string_ptr;
      string_length = string_hdr->s_length;
      string_cell = string_hdr->s_head;
      if (string_cell == NULL) {
         string_char_ptr = string_char_end = NULL;
      }
      else {
         string_char_ptr = string_cell->s_cell_value;
         string_char_end = string_char_ptr + STR_CELL_WIDTH;
      }

      /* allocate a C buffer */

      return_string = (char *)malloc((size_t)(string_length + 1));
      if (return_string == NULL)
         abend(SETL_SYSTEM msg_malloc_error);
      p = return_string;

      /* copy the string into the buffer */

      while (string_length--) {

         if (string_char_ptr == string_char_end) {

            string_cell = string_cell->s_next;
            string_char_ptr = string_cell->s_cell_value;
            string_char_end = string_char_ptr + STR_CELL_WIDTH;

         }

         *p++ = *string_char_ptr++;

      }

      *p = '\0';

   }
   else if (spare.sp_form != ft_omega) {

      abend(SETL_SYSTEM "Expected string or om return from callback, but found:\n %s",
            abend_opnd_str(SETL_SYSTEM &spare));

   }

   unmark_specifier(&spare);

   return return_string;

}

/*\
 *  \function{setl2\_Ccallout2()}
 *
 *  This is the entry function for callout.  The user passes an integer
 *  service code, a callback handler, and a tuple of arguments for C.  We
 *  convert these arguments to C form and pass them to a user-provided
 *  function.
\*/

void setl2_Ccallout2(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (3 here)          */
   specifier *target)                  /* return value                      */

{
int function_type;                     /* output function code              */
int count;                             /* number of arguments               */
int total_length;                      /* total string length               */
struct comm_block *save_comm_block;    /* block used by this instance       */
char *arg_string;                      /* argument storage string           */
char **arg_vector;                     /* callout argument vector           */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* tuple element                     */
string_h_ptr_type string_hdr;
                                       /* source and target strings         */
string_c_ptr_type string_cell;
                                       /* source and target string cells    */
int32 string_length;                   /* source string length              */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
struct return_struct *rs;              /* returned structure                */
char *p;                               /* temporary looping variable        */

   /* the first argument must be an integer */

   if (argv[0].sp_form == ft_short) {

      function_type = (int)(argv[0].sp_val.sp_short_value);

   }
   else if (argv[0].sp_form == ft_long) {

      function_type = (int)long_to_short(SETL_SYSTEM argv[0].sp_val.sp_long_ptr);

   }
   else {

      abend(SETL_SYSTEM msg_bad_arg,"integer",1,"callout2",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   /* the second must be the callback handler (don't check now) */

   callback2.sp_form = argv[1].sp_form;
   callback2.sp_val.sp_biggest = argv[1].sp_val.sp_biggest;
   mark_specifier(&callback2);

   /* the last must be a tuple of strings */

   if (argv[2].sp_form != ft_tuple)
      abend(SETL_SYSTEM msg_bad_arg,"tuple",3,"callout",
            abend_opnd_str(SETL_SYSTEM argv+2));

   /* set up to loop over the tuple, counting arguments and lengths */

   count = 0;
   total_length = 0;
   source_root = argv[2].sp_val.sp_tuple_ptr;
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

      /* we expect each element of the tuple to be a string */

      if (count < source_number)
         abend(SETL_SYSTEM msg_bad_arg,"tuple of strings",3,"callout",
               abend_opnd_str(SETL_SYSTEM argv+2));

      if (source_element->sp_form != ft_string)
         abend(SETL_SYSTEM msg_bad_arg,"tuple of strings",3,"callout",
               abend_opnd_str(SETL_SYSTEM argv+2));

      count++;
      total_length += (source_element->sp_val.sp_string_ptr)->s_length + 1;

   }

   /* we've calculated how much space we need, now we allocate it */

   if (next_comm_block == NULL) {

      next_comm_block = 
         (struct comm_block *)malloc(sizeof(struct comm_block));
      if (next_comm_block == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);

      comm_list_tail = &(next_comm_block->cb_next);
      next_comm_block->cb_ptr_length = 0;
      next_comm_block->cb_ptr = NULL;
      next_comm_block->cb_str_length = 0;
      next_comm_block->cb_str = NULL;
      next_comm_block->cb_next = NULL;

   }     

   if (next_comm_block->cb_ptr_length < count) {

      if (next_comm_block->cb_ptr != NULL) 
         free(next_comm_block->cb_ptr);
      next_comm_block->cb_ptr =
         (char **)malloc((size_t)(count * sizeof(char *)));
      if (next_comm_block->cb_ptr == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      next_comm_block->cb_ptr_length = count;

   }

   if (next_comm_block->cb_str_length < total_length) {

      if (next_comm_block->cb_str != NULL) 
         free(next_comm_block->cb_str);
      next_comm_block->cb_str =
         (char *)malloc((size_t)total_length);
      if (next_comm_block->cb_str == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      next_comm_block->cb_str_length = total_length;

   }

   arg_vector = next_comm_block->cb_ptr;
   arg_string = next_comm_block->cb_str;
   save_comm_block = next_comm_block;
   next_comm_block = next_comm_block->cb_next;

   /* loop over the tuple again, building up the array for C */

   p = arg_string;
   count = 0;

   source_root = argv[2].sp_val.sp_tuple_ptr;
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

      /* we have an element, copy the string into our buffer */

      arg_vector[count] = p;
      count++;

      /* initialize the source string */

      string_hdr = source_element->sp_val.sp_string_ptr;
      string_length = string_hdr->s_length;
      string_cell = string_hdr->s_head;
      if (string_cell == NULL) {
         string_char_ptr = string_char_end = NULL;
      }
      else {
         string_char_ptr = string_cell->s_cell_value;
         string_char_end = string_char_ptr + STR_CELL_WIDTH;
      }

      /* copy the string into the buffer */

      while (string_length--) {

         if (string_char_ptr == string_char_end) {

            string_cell = string_cell->s_next;
            string_char_ptr = string_cell->s_cell_value;
            string_char_end = string_char_ptr + STR_CELL_WIDTH;

         }

         *p++ = *string_char_ptr++;

      }

      *p++ = '\0';

   }

   /* finally, we're ready to call the C version of this function */

   rs = setl2_callout2(SETL_SYSTEM function_type,count,arg_vector);

   /* we have to create a return value for SETL2 */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = NULL;
   string_char_ptr = string_char_end = NULL;

   /* copy the return string */

   if (rs != NULL) {

      p = rs->rs_data;
      while (rs->rs_length--) {

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

         *string_char_ptr++ = *p++;
         string_hdr->s_length++;

      }
   }

   /* set the return value */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   /* free our string buffers */

   next_comm_block = save_comm_block;
   unmark_specifier(&callback);

   return;

}

/*\
 *  \function{setl2\_callback2()}
 *
 *  This function handles callbacks from C.  We convert the arguments
 *  from C to \setl\ form, then execute the \setl\ procedure.
\*/

#if UNIX_VARARGS
char *setl2_callback2(va_alist)
   va_dcl                              /* all arguments                     */
#else
char *setl2_callback2(
   SETL_SYSTEM_PROTO
   struct return_struct *firstarg,     /* first argument from caller        */
   ...)                                /* any other arguments               */
#endif

{
va_list argp;                          /* current argument pointer          */
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
int32 string_length;                   /* source string length              */
tuple_h_ptr_type tuple_root, tuple_work_hdr, new_tuple_hdr;
                                       /* tuple header pointers             */
tuple_c_ptr_type tuple_cell;           /* tuple cell pointer                */
int32 tuple_length,tuple_height,tuple_index;
                                       /* used to create return tuple       */
static char *return_string = NULL;     /* static return buffer              */
int32 expansion_trigger,work_hash_code;
                                       /* used to build tuple               */
int first_arg;                         /* YES if first argument             */
specifier spare;                       /* spare specifier                   */
specifier save_callback;               /* saved callback handler            */
struct return_struct *rs;              /* return structure                  */
char *p;                               /* temporary looping varible         */
int i;                                 /* temporary looping variable        */

   /* make sure our callback is a procedure */

   if (callback2.sp_form != ft_proc)
      abend(SETL_SYSTEM "Expected procedure in callout2, but found:\n %s",
            abend_opnd_str(SETL_SYSTEM &callback2));

   /* create an empty tuple */

   get_tuple_header(tuple_root);
   tuple_root->t_use_count = 1;
   tuple_root->t_hash_code = 0;
   tuple_root->t_ntype.t_root.t_length = 0;
   tuple_root->t_ntype.t_root.t_height = 0;

   for (i = 0;
        i < TUP_HEADER_SIZE;
        tuple_root->t_child[i++].t_cell = NULL);

   /* insert each element in the tuple */

   tuple_length = 0;
   first_arg = YES;
#if UNIX_VARARGS
   va_start(argp);
   rs = va_arg(argp, struct return_struct *);
#else
   va_start(argp,firstarg);
   rs = firstarg;
#endif
   while (rs != NULL) {

      /* first we make a SETL2 string out of the C string */

      get_string_header(string_hdr);
      string_hdr->s_use_count = 1;
      string_hdr->s_hash_code = -1;
      string_hdr->s_length = 0;
      string_hdr->s_head = string_hdr->s_tail = NULL;
      string_char_ptr = string_char_end = NULL;

      /* copy the source string */

      p = rs->rs_data;
      while (rs->rs_length--) {

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

         *string_char_ptr++ = *p++;
         string_hdr->s_length++;

      }

      /*
       *  We've converted one of the C arguments into a SETL2 string.
       *  We push the first argument, and append others to our tuple.
       */

      if (first_arg) {

         first_arg = NO;
         spare.sp_form = ft_string;
         spare.sp_val.sp_string_ptr = string_hdr;
         push_pstack(&spare);
         string_hdr->s_use_count--;

         rs = va_arg(argp, struct return_struct *);
         continue;

      }

      /* add it to the tuple */

      expansion_trigger = 1 << ((tuple_root->t_ntype.t_root.t_height + 1)
                                 * TUP_SHIFT_DIST);

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

      }

      tuple_root->t_ntype.t_root.t_length = tuple_length + 1;

      /* find the tuple component */

      tuple_work_hdr = tuple_root;
      for (tuple_height = tuple_root->t_ntype.t_root.t_height;
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
            new_tuple_hdr->t_ntype.t_intern.t_child_index = (int)tuple_index;
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
      tuple_work_hdr->t_child[tuple_index].t_cell = tuple_cell;
      tuple_cell->t_spec.sp_form = ft_string;
      tuple_cell->t_spec.sp_val.sp_string_ptr = string_hdr;
      spec_hash_code(work_hash_code,&(tuple_cell->t_spec));
      tuple_root->t_hash_code ^= work_hash_code;
      tuple_cell->t_hash_code = work_hash_code;

      /* get the next argument */

      tuple_length++;
      rs = va_arg(argp, struct return_struct *);

   }

   /* if we didn't get any arguments, push an omega */

   if (first_arg) {

      spare.sp_form = ft_omega;
      push_pstack(&spare);

   }

   /* push our tuple */

   spare.sp_form = ft_tuple;
   spare.sp_val.sp_tuple_ptr = tuple_root;
   push_pstack(&spare);
   tuple_root->t_use_count--;

   /* call the callback handler */

   save_callback.sp_form = callback2.sp_form;
   save_callback.sp_val.sp_biggest = callback2.sp_val.sp_biggest;
   spare.sp_form = ft_omega;
   call_procedure(SETL_SYSTEM &spare,
                  &save_callback,
                  NULL,
                  2L,YES,NO,0);
   callback2.sp_form = save_callback.sp_form;
   callback2.sp_val.sp_biggest = save_callback.sp_val.sp_biggest;

   /* release any old return string */

   if (return_string != NULL) {
      free(return_string);
      return_string = NULL;
   }

   /* build up the C string */

   if (spare.sp_form == ft_string) {

      /* initialize the source string */

      string_hdr = spare.sp_val.sp_string_ptr;
      string_length = string_hdr->s_length;
      string_cell = string_hdr->s_head;
      if (string_cell == NULL) {
         string_char_ptr = string_char_end = NULL;
      }
      else {
         string_char_ptr = string_cell->s_cell_value;
         string_char_end = string_char_ptr + STR_CELL_WIDTH;
      }

      /* allocate a C buffer */

      return_string = (char *)malloc((size_t)(string_length + 1));
      if (return_string == NULL)
         abend(SETL_SYSTEM msg_malloc_error);
      p = return_string;

      /* copy the string into the buffer */

      while (string_length--) {

         if (string_char_ptr == string_char_end) {

            string_cell = string_cell->s_next;
            string_char_ptr = string_cell->s_cell_value;
            string_char_end = string_char_ptr + STR_CELL_WIDTH;

         }

         *p++ = *string_char_ptr++;

      }

      *p = '\0';

   }
   else if (spare.sp_form != ft_omega) {

      abend(SETL_SYSTEM "Expected string or om return from callback, but found:\n %s",
            abend_opnd_str(SETL_SYSTEM &spare));

   }

   unmark_specifier(&spare);

   return return_string;

}

void setl2_malloc(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
int amount;
void *area;
char load_result[16];

 /* the first argument must be an integer */

   if (argv[0].sp_form == ft_short) {

      amount = (int)(argv[0].sp_val.sp_short_value);

   }
   else if (argv[0].sp_form == ft_long) {

      amount = (int)long_to_short(SETL_SYSTEM argv[0].sp_val.sp_long_ptr);

   }
   else {

      abend(SETL_SYSTEM msg_bad_arg,"integer",1,"malloc",
            abend_opnd_str(SETL_SYSTEM argv));

   }

   area=(void *)malloc(amount);
    
   if (area==NULL) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   strcpy(load_result,"");
#ifdef MACINTOSH
   sprintf(load_result,"%ld",(long)area);
#endif
#if UNIX
   sprintf(load_result,"%p",area);
#endif
   s=load_result;

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

void setl2_dispose(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *key;                             /* system key                        */
char *s, *t;                           /* temporary looping variables       */
int close_result;
void *area;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"free",
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

#ifdef MACINTOSH
   sscanf(key,"%ld",&area);
#endif

#if UNIX
   sscanf(key,"%p",&area);
#endif
   
   free(key);
   free(area);

   unmark_specifier(target);
   target->sp_form = ft_omega;
   return;

}

void setl2_open_lib(
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
char load_result[16];

#ifdef MACINTOSH
OSErr myErr;
FSSpec myFSSpec;
CFragConnectionID myConnID;
Ptr myMainAddr;
Str255 myErrName;
Str63 fragName;
#endif

#if HAVE_DLFCN_H
void *handle;
#endif

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"dll_open",
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

   strcpy(load_result,"");
	
#ifdef MACINTOSH
   myErr = GetSharedLibrary (c2pstr(key),
		             kPowerPCCFragArch,
			     kPrivateCFragCopy,
		  	     &myConnID,
			     &myMainAddr,
			     myErrName);	
   if (!myErr)
      sprintf(load_result,"%ld",(long)myConnID);
#endif

#if HAVE_DLFCN_H
   handle=dlopen(key,RTLD_LAZY);
   if (handle!=NULL)
      sprintf(load_result,"%p",handle);
#endif   
 
   free(key);

   if (strcmp(load_result,"")==0) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   s=load_result;

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

void setl2_close_lib(
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
int close_result;

#ifdef MACINTOSH  
CFragConnectionID 	myConnID;
#endif

#if HAVE_DLFCN_H
void *handle;
#endif

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"dll_close",
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

   
   close_result=-1;
	
#ifdef MACINTOSH
   sscanf(key,"%ld",&myConnID);
   close_result=CloseConnection(&myConnID);
#endif

#if HAVE_DLFCN_H
   sscanf(key,"%p",&handle);
   close_result=dlclose(handle);
#endif

   free(key);

   if (close_result!=0) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value = 0;

}

void setl2_find_symbol(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
char *handle;                          /* system key                        */
char *symbol;                          /* system key                        */
char symbol_pointer[16];
int call_result;

#if HAVE_DLFCN_H
void *phandle;
void *psymb;
#endif

#ifdef MACINTOSH
OSErr myErr;
FSSpec myFSSpec;
CFragConnectionID myConnID;
Ptr myMainAddr;
Str255 myErrName;
Str63 fragName;
Str255 myName;
long myCount;
CFragSymbolClass myClass;
Ptr myAddr;
#endif

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"dll_findsymbol",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   handle = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (handle == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = handle;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < handle + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"dll_findsymbol",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[1].sp_val.sp_string_ptr;

   symbol = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (symbol == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = symbol;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < symbol + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';
   
   strcpy(symbol_pointer,"");

#ifdef MACINTOSH
   sscanf(handle,"%ld",&myConnID);

   myErr = FindSymbol(myConnID,
		      c2pstr(symbol),
		      &myAddr, &myClass);

   if (!myErr) 
      sprintf(symbol_pointer,"%ld",(long)myAddr);
#endif

#if HAVE_DLFCN_H
   sscanf(handle,"%p",&phandle);
   psymb=dlsym(phandle,symbol);
   if (psymb!=NULL)
      sprintf(symbol_pointer,"%p",psymb);
#endif
 
   free(handle);
   free(symbol);

   if (strcmp(symbol_pointer,"")==0) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   s=symbol_pointer;

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


void setl2_call_function(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
char function_type[16];                /*                                   */
int typelen;
int count;                             /* number of arguments               */
int total_length;                      /* total string length               */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* tuple element                     */
string_h_ptr_type string_hdr;
                                       /* source and target strings         */
string_c_ptr_type string_cell;
                                       /* source and target string cells    */
int32 string_length;                   /* source string length              */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *p;                               /* temporary looping variable        */
char *s, *t, *t2;                      /* temporary looping variables       */
char *key;
int rint;
float rfloat;
double rdouble;
i_real_ptr_type real_ptr;              /* real pointer                      */


int arg_vector_int[10];
float arg_vector_float[10];
double arg_vector_double[10];

void *Fp;

char symbol_pointer[16];
char void_pointer[16];
int ftype;
int foundint;
int which_one;
int rtype;

#if UNIX
void *psymb;
void *vp;
#endif

#ifdef MACINTOSH
Ptr myAddr;
Ptr vp;
#endif

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"callfunction",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   typelen= string_hdr->s_length;

   if (typelen > 10 )
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"callfunction",
            abend_opnd_str(SETL_SYSTEM argv));

   t = function_type;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < function_type + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   ftype=0;
   foundint=0;
   which_one=0;
   for (t = function_type; *t; t++) {
      if (islower(*t))
         *t = toupper(*t);

      if (t==function_type) s="VIFDP"; else s="IFDPS";
      if (strchr(s,*t)==NULL)
        abend(SETL_SYSTEM msg_bad_arg,"string",1,"callfunction",abend_opnd_str(SETL_SYSTEM argv));

      if (t==function_type) {  /* Return Value */
         rtype=(strchr(s,*t)-s+1); 
         which_one=rtype*1000; 
         if (rtype==5) which_one=2000;
         continue;
      }

      if ((*t=='I')||(*t=='P')||(*t=='S')) {
         foundint=1;
         which_one++;
         continue;
      }

      which_one +=11;

      if (ftype==0) {
         if (foundint!=0) /* Floats after INTs... */
            abend(SETL_SYSTEM msg_bad_arg,"string",1,"callfunction",abend_opnd_str(SETL_SYSTEM argv));
         ftype=*t;
         if (*t=='D') which_one +=400; else which_one+=300;
      } else 
         if (ftype!=*t)
            abend(SETL_SYSTEM msg_bad_arg,"string",1,"callfunction",abend_opnd_str(SETL_SYSTEM argv));
   }


   /* the second argument must be a string */
   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"callfunction",
            abend_opnd_str(SETL_SYSTEM argv+1));

   string_hdr = argv[1].sp_val.sp_string_ptr;

   if (string_hdr->s_length >10 )
      abend(SETL_SYSTEM msg_bad_arg,"string",2,"callfunction",
            abend_opnd_str(SETL_SYSTEM argv+1));

   t = symbol_pointer;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < symbol_pointer + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

#ifdef MACINTOSH  
   sscanf(symbol_pointer,"%ld",&Fp);
#endif
#if UNIX
   sscanf(symbol_pointer,"%p",&Fp);
#endif

   /* the last must be a tuple of strings */

   if (argv[2].sp_form != ft_tuple)
      abend(SETL_SYSTEM msg_bad_arg,"tuple",3,"callfunction",
            abend_opnd_str(SETL_SYSTEM argv+2));

   /* set up to loop over the tuple, counting arguments and lengths */

   count = 0;
   total_length = 0;
   source_root = argv[2].sp_val.sp_tuple_ptr;
   source_work_hdr = source_root;
   source_number = -1;
   source_height = source_root->t_ntype.t_root.t_height;
   source_index = 0;

   /* loop over the elements of source */

   t=function_type+1; /* Points to the type of the parameters */

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

      /* we expect each element of the tuple to be a string */

      if (count < source_number)
         abend(SETL_SYSTEM msg_bad_arg,"tuple of strings",3,"callfunction",
               abend_opnd_str(SETL_SYSTEM argv+2));

      switch (*t) {
         case 'S':  /* String */
                 /* the argument must be a string */
            if (source_element->sp_form != ft_string)
               abend(SETL_SYSTEM msg_bad_arg,"string",3,"callfunction",
                     abend_opnd_str(SETL_SYSTEM source_element));

            string_hdr = source_element->sp_val.sp_string_ptr;
            key = (char *)malloc((size_t)(string_hdr->s_length + 1));
            if (key == NULL)
               giveup(SETL_SYSTEM msg_malloc_error);

            t2 = key;
            for (string_cell = string_hdr->s_head;
                 string_cell != NULL;
                 string_cell = string_cell->s_next) {

               for (s = string_cell->s_cell_value;
                    t2 < key + string_hdr->s_length &&
                       s < string_cell->s_cell_value + STR_CELL_WIDTH;
                    *t2++ = *s++);

            }
            *t2 = '\0';
            arg_vector_int[count]=(int32)(key);
            break;
         case 'P':  /* Pointer */
                 /* the argument must be a string */
            if (source_element->sp_form != ft_string)
               abend(SETL_SYSTEM msg_bad_arg,"string",3,"callfunction",
                     abend_opnd_str(SETL_SYSTEM source_element));

            string_hdr = source_element->sp_val.sp_string_ptr;

            t2 = void_pointer;
            for (string_cell = string_hdr->s_head;
                 string_cell != NULL;
                 string_cell = string_cell->s_next) {

               for (s = string_cell->s_cell_value;
                    t2 < void_pointer + string_hdr->s_length &&
                       s < string_cell->s_cell_value + STR_CELL_WIDTH;
                    *t2++ = *s++);

            }
            *t2 = '\0';
#ifdef MACINTOSH  
            sscanf(void_pointer,"%ld",&vp);
            arg_vector_int[count]=(int)(vp);
#endif
#if UNIX
            sscanf(void_pointer,"%p",&vp);
            arg_vector_int[count]=(int)(vp);
#endif
            break;
         case 'I':
            if (source_element->sp_form == ft_short) {
               arg_vector_int[count] = 
                      (int)(source_element->sp_val.sp_short_value);

            }
            else if (source_element->sp_form == ft_long) {
               arg_vector_int[count] = 
                      (int)long_to_short(SETL_SYSTEM source_element->sp_val.sp_long_ptr);
            }
            else {
               abend(SETL_SYSTEM msg_bad_arg,"integer",3,"callfunction",
                  abend_opnd_str(SETL_SYSTEM source_element));
            }
            break;
         case 'F':
            switch (source_element->sp_form) {

                  case ft_short :

                     arg_vector_float[count] = 
                            (float)(source_element->sp_val.sp_short_value);
                     break;

                  case ft_long :

                     arg_vector_float[count] = 
                            (float)long_to_double(SETL_SYSTEM source_element);
                     break;

                  case ft_real :
                     arg_vector_float[count] = 
                            (float)((source_element->sp_val.sp_real_ptr)->r_value);
                     break;
                     

                  default :
                     abend(SETL_SYSTEM msg_bad_arg,"float",3,"callfunction",
                               abend_opnd_str(SETL_SYSTEM source_element));

            }
         case 'D':
            switch (source_element->sp_form) {

                  case ft_short :

                     arg_vector_double[count] = 
                            (double)(source_element->sp_val.sp_short_value);
                     break;

                  case ft_long :

                     arg_vector_double[count] = 
                            (double)long_to_double(SETL_SYSTEM source_element);
                     break;

                  case ft_real :
                     arg_vector_double[count] = 
                            (double)((source_element->sp_val.sp_real_ptr)->r_value);
                     break;
                     

                  default :
                     abend(SETL_SYSTEM msg_bad_arg,"double",3,"callfunction",
                               abend_opnd_str(SETL_SYSTEM source_element));

            }

      }

      count++;
      t++;

   }

   if ((count+1) != typelen)
         abend(SETL_SYSTEM msg_bad_arg,"tuple of C parameters",3,"callfunction",
               abend_opnd_str(SETL_SYSTEM argv+2));

   /* finally, we're ready to call the C version of this function */
   switch(which_one) {

#include "callinc.h"

      default:
        abend(SETL_SYSTEM msg_bad_arg,"tuple of C parameters",1,"callfunction",abend_opnd_str(SETL_SYSTEM argv+2));

   }

   t2=function_type+1;
   for (count=1;count<typelen;count++) {
     if (*t2=='S') free((void *)arg_vector_int[count-1]);
     t2++;
   }

   
   if (rtype==1) { /* Void return type */
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   /* we have to create a return value for SETL2 */
   switch (rtype) {
      case 5:
         break;
      case 2:
         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = rint;
         return;
      case 3:
         unmark_specifier(target);
         i_get_real(real_ptr);
         target->sp_form = ft_real;
         target->sp_val.sp_real_ptr = real_ptr;
         real_ptr->r_use_count = 1;
         real_ptr->r_value = (double)rfloat;
         return;
      case 4:
         unmark_specifier(target);
         i_get_real(real_ptr);
         target->sp_form = ft_real;
         target->sp_val.sp_real_ptr = real_ptr;
         real_ptr->r_use_count = 1;
         real_ptr->r_value = rdouble;
         return;
   }

   strcpy(void_pointer,"");
#ifdef MACINTOSH
   sprintf(void_pointer,"%ld",(long)rint);
#endif

#if UNIX
   sprintf(void_pointer,"%p",(void *)rint);
#endif

   p=void_pointer;
   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = NULL;
   string_char_ptr = string_char_end = NULL;

   /* copy the return string */

   if (p != NULL) {

      while (*p) {

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

         *string_char_ptr++ = *p++;
         string_hdr->s_length++;

      }
   }

   /* set the return value */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;

}

void setl2_num_symbols(
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
int num_symbs;

#ifdef MACINTOSH
OSErr   myErr;
long    myCount;
CFragConnectionID 	myConnID;
#endif

#if HAVE_DLFCN_H
void *handle;
#endif

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"dll_numsymbols",
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

   
   num_symbs=-1;
	
#ifdef MACINTOSH
   sscanf(key,"%ld",&myConnID);
   myErr = CountSymbols(myConnID, &myCount);
   if (myErr==0) num_symbs=myCount;
#endif

   free(key);

   if (num_symbs==-1) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value = num_symbs;

}


void setl2_get_symbol(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
char *handle;                          /* system key                        */
int symbol;                            /* system key                        */
char symbol_pointer[16];
int call_result;

#if HAVE_DLFCN_H
void *phandle;
void *psymb;
#endif

#ifdef MACINTOSH
OSErr myErr;
FSSpec myFSSpec;
CFragConnectionID myConnID;
Ptr myMainAddr;
Str255 myErrName;
Str63 fragName;
Str255 myName;
long myCount;
CFragSymbolClass myClass;
Ptr myAddr;
#endif

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"dll_getsymbol",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   handle = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (handle == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = handle;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < handle + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   if (argv[1].sp_form == ft_short) {
      symbol = (int)(argv[1].sp_val.sp_short_value);

   }
   else if (argv[1].sp_form == ft_long) {
      symbol = (int)long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"dll_getsymbol",
         abend_opnd_str(SETL_SYSTEM &argv[1]));
   }
   
   strcpy(symbol_pointer,"");

#ifdef MACINTOSH  
   sscanf(handle,"%ld",&myConnID);

   myErr = GetIndSymbol(myConnID, (long)symbol, myName, &myAddr, &myClass);

   if (!myErr) 
      sprintf(symbol_pointer,"%ld",(long)myAddr);
#endif

   free(handle);

   if (strcmp(symbol_pointer,"")==0) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   s=symbol_pointer;

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

void setl2_get_symbol_name(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
char *handle;                          /* system key                        */
int symbol;                            /* system key                        */
char symbol_pointer[16];
int call_result;
char *c_stg; 

#if HAVE_DLFCN_H
void *phandle;
void *psymb;
#endif

#ifdef MACINTOSH
OSErr myErr;
FSSpec myFSSpec;
CFragConnectionID myConnID;
Ptr myMainAddr;
Str255 myErrName;
Str63 fragName;
Str255 myName;
long myCount;
CFragSymbolClass myClass;
Ptr myAddr;
int stg_len;
char *stg_copy;
#endif

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"dll_getsymbolname",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   handle = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (handle == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = handle;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < handle + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   if (argv[1].sp_form == ft_short) {
      symbol = (int)(argv[1].sp_val.sp_short_value);

   }
   else if (argv[1].sp_form == ft_long) {
      symbol = (int)long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"dll_getsymbolname",
         abend_opnd_str(SETL_SYSTEM &argv[1]));
   }
   
   strcpy(symbol_pointer,"");
   c_stg=symbol_pointer;

#ifdef MACINTOSH  

   sscanf(handle,"%ld",&myConnID);
   myErr = GetIndSymbol(myConnID, (long)symbol, myName, &myAddr, &myClass);

   if (!myErr)  {
      c_stg = (char *)p2cstr(myName); 

   }
#endif

   free(handle);

   if (strcmp(c_stg,"")==0) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }

   s=c_stg;

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
 
void setl2_bpeek(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
char *location;  
void *plocation;
int offset;  
char b;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"peek",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   location = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (location == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = location;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < location + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   if (argv[1].sp_form == ft_short) {
      offset = (int)(argv[1].sp_val.sp_short_value);

   }
   else if (argv[1].sp_form == ft_long) {
      offset = (int)long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"peek",
         abend_opnd_str(SETL_SYSTEM &argv[1]));
   }
   
#ifdef MACINTOSH  
   sscanf(location,"%ld",&plocation);
#endif

#ifdef UNIX
   sscanf(location,"%p",&plocation);
#endif

   free(location);
   /* plocation and offset now contain the correct values */

   b=*(char *)((char *)(plocation)+offset);
   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value = (int)b;
   return;

}
void setl2_speek(
   SETL_SYSTEM_PROTO

   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
char *location;  
void *plocation;
int offset;  
short b;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"peek",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   location = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (location == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = location;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < location + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   if (argv[1].sp_form == ft_short) {
      offset = (int)(argv[1].sp_val.sp_short_value);

   }
   else if (argv[1].sp_form == ft_long) {
      offset = (int)long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"peek",
         abend_opnd_str(SETL_SYSTEM &argv[1]));
   }
   
#ifdef MACINTOSH  
   sscanf(location,"%ld",&plocation);
#endif

#ifdef UNIX
   sscanf(location,"%p",&plocation);
#endif

   free(location);
   /* plocation and offset now contain the correct values */


   b=*(short *)((char *)(plocation)+offset);
   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value = (int)b;
   return;

}
void setl2_ipeek(

   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
char *location;  
void *plocation;
int offset;  
int32 b;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"peek",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   location = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (location == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = location;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < location + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   if (argv[1].sp_form == ft_short) {
      offset = (int)(argv[1].sp_val.sp_short_value);

   }
   else if (argv[1].sp_form == ft_long) {
      offset = (int)long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"peek",
         abend_opnd_str(SETL_SYSTEM &argv[1]));
   }
   
#ifdef MACINTOSH  
   sscanf(location,"%ld",&plocation);
#endif

#ifdef UNIX
   sscanf(location,"%p",&plocation);
#endif

   free(location);
   /* plocation and offset now contain the correct values */

   b=*(int32 *)((char *)(plocation)+offset);
   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value = (int)b;
   return;

}
void setl2_bpoke(

   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
char *location;  
void *plocation;
int offset;  
char b;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"poke",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   location = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (location == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = location;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < location + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   if (argv[1].sp_form == ft_short) {
      offset = (int)(argv[1].sp_val.sp_short_value);

   }
   else if (argv[1].sp_form == ft_long) {
      offset = (int)long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"poke",
         abend_opnd_str(SETL_SYSTEM &argv[1]));
   }

   if (argv[2].sp_form == ft_short) {
      b = (char)(argv[2].sp_val.sp_short_value);

   }
   else if (argv[2].sp_form == ft_long) {
      b = (char)long_to_short(SETL_SYSTEM argv[2].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",3,"poke",
         abend_opnd_str(SETL_SYSTEM &argv[2]));
   }
   
#ifdef MACINTOSH  
   sscanf(location,"%ld",&plocation);
#endif

#ifdef UNIX
   sscanf(location,"%p",&plocation);
#endif

   free(location);
   /* plocation and offset now contain the correct values */
 
   *(char *)((char *)(plocation)+offset)=b;

   return;
}

void setl2_spoke(

   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
char *location;  
void *plocation;
int offset;  
short b;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"poke",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   location = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (location == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = location;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < location + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   if (argv[1].sp_form == ft_short) {
      offset = (int)(argv[1].sp_val.sp_short_value);

   }
   else if (argv[1].sp_form == ft_long) {
      offset = (int)long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"poke",
         abend_opnd_str(SETL_SYSTEM &argv[1]));
   }

   if (argv[2].sp_form == ft_short) {
      b = (short)(argv[2].sp_val.sp_short_value);

   }
   else if (argv[2].sp_form == ft_long) {
      b = (short)long_to_short(SETL_SYSTEM argv[2].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",3,"poke",
         abend_opnd_str(SETL_SYSTEM &argv[2]));
   }
   
#ifdef MACINTOSH  
   sscanf(location,"%ld",&plocation);
#endif

#ifdef UNIX
   sscanf(location,"%p",&plocation);
#endif

   free(location);
   /* plocation and offset now contain the correct values */
 
   *(short *)((char *)(plocation)+offset)=b;

   return;
}
void setl2_ipoke(

   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
char *location;  
void *plocation;
int offset;  
int32 b;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"poke",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   location = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (location == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = location;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < location + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   if (argv[1].sp_form == ft_short) {
      offset = (int)(argv[1].sp_val.sp_short_value);

   }
   else if (argv[1].sp_form == ft_long) {
      offset = (int)long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"poke",
         abend_opnd_str(SETL_SYSTEM &argv[1]));
   }

   if (argv[2].sp_form == ft_short) {
      b = (int32)(argv[2].sp_val.sp_short_value);

   }
   else if (argv[2].sp_form == ft_long) {
      b = (int32)long_to_short(SETL_SYSTEM argv[2].sp_val.sp_long_ptr);
   }
   else {
      abend(SETL_SYSTEM msg_bad_arg,"integer",3,"poke",
         abend_opnd_str(SETL_SYSTEM &argv[2]));
   }
   
#ifdef MACINTOSH  
   sscanf(location,"%ld",&plocation);
#endif

#ifdef UNIX
   sscanf(location,"%p",&plocation);
#endif

   free(location);
   /* plocation and offset now contain the correct values */
 
   *(int32 *)((char *)(plocation)+offset)=b;

   return;
}



void setl2_host_get(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;
                                       /* source string pointers            */
char *s, *t;                           /* temporary looping variables       */
char *property;  
void *plocation;
int offset;  
int32 b;

#ifdef SPAM

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"host_get",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   property = (char *)malloc((size_t)(string_hdr->s_length + 1));
   if (property == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = property;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < location + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';



   free(property);
   return;

#else
   target->sp_form = ft_omega;
   return;
#endif

}


void setl2_host_put(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
   target->sp_form = ft_omega;
   return;
}


void setl2_host_call(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
   target->sp_form = ft_omega;
   return;
}

void setl2_reset_callback(SETL_SYSTEM_PROTO_VOID)
{
	callback.sp_form = ft_void;
}
