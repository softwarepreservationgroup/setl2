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
 *  \package{The Abnormal End Handler}
 *
 *  This package handle abnormal program terminations.  The goal here is
 *  to print information usable to the programmer in debugging his
 *  program.  We are not concerned at all with speed here, nor are we
 *  worried about wasting space.  We just want to print something useful
 *  and kill the program.
 *
 *  \texify{abend.h}
 *
 *  \packagebody{Abnormal End Handler}
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
#include "libman.h"                    /* library manager                   */
#include "chartab.h"                   /* character type table              */
#include "builtins.h"                  /* built-in symbols                  */
#include "unittab.h"                   /* unit table                        */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_strngs.h"                  /* strings                           */

#ifdef PLUGIN
#define fprintf plugin_fprintf
#define printf plugin_printf
#define exit(a) plugin_exit(SETL_SYSTEM a)
#endif

#include <setjmp.h>
extern jmp_buf abend_env;
extern int hard_stop;
extern int abend_initialized;


/* forward declarations */

static void find_position(SETL_SYSTEM_PROTO unittab_ptr_type, 
                          int, instruction *,
                          char *, int32 *, int32 *);
                                       /* find file position of instruction */


/*\
 *  \function{abend\_opnd\_str()}
 *
 *  This function creates a somewhat readable form of a specifier,
 *  including at least its form, and part of its printable string.  We
 *  truncate that string at 70 characters.
\*/

char *abend_opnd_str(
   SETL_SYSTEM_PROTO
   specifier *spec)                    /* first argument of message         */

{
char *return_ptr;                      /* returned string                   */
specifier spare;                       /* spare specifier for str call      */
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
int chars_to_print;                    /* characters left in string         */
int print_dots;                        /* YES if we truncated the string    */
char *s, *t;                           /* temporary looping variables       */

   /* find the printable string form of the specifier */

   spare.sp_form = ft_omega;
   setl2_str(SETL_SYSTEM 1, spec, &spare);

   /* allocate a return string */

   string_hdr = spare.sp_val.sp_string_ptr;
   return_ptr = (char *)malloc((size_t)71);
   if (return_ptr == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* the first part of the string is its form */

   strcpy(return_ptr,form_desc[spec->sp_form]);
   t = return_ptr + strlen(return_ptr);
   *t++ = ':';
   *t++ = ' ';

   /* adjust the number of characters we print */

   chars_to_print = 66 - (t - return_ptr);
   if (chars_to_print >= string_hdr->s_length) {
      chars_to_print = (int)(string_hdr->s_length);
      print_dots = NO;
   }
   else {
      print_dots = YES;
   }

   /* copy the string (or a prefix) */

   for (string_cell = string_hdr->s_head;
        chars_to_print;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           chars_to_print && (s < string_cell->s_cell_value + STR_CELL_WIDTH);
           *t++ = *s++, chars_to_print--);

   }

   /* print dots if we truncated the string */

   if (print_dots) {
      strcpy(t," ...");
   }
   else {
      *t = '\0';
   }

   return return_ptr;

}

/*\
 *  \function{abend()}
 *
 *  This function handles abnormal end of jobs.  We just want to print
 *  some information to help the programmer.  We rely on the caller to
 *  send an appropriate message.  Here we print file locations and call
 *  stack.
\*/

#if UNIX_VARARGS
void abend(message,va_alist)
   char *message;                      /* first argument of message         */
   va_dcl                              /* any other arguments               */
#else
void abend(
   SETL_SYSTEM_PROTO
   char *message,                      /* first argument of message         */
   ...)                                /* any other arguments               */
#endif

{
va_list arg_pointer;                   /* argument list pointer             */

#ifdef TSAFE
#define ABEND__SOURCE_NAME plugin_instance->abend__source_name
#else
#define ABEND__SOURCE_NAME source_name
static char source_name[PATH_LENGTH + 1];
#endif
                                       /* source file name                  */
int32 error_line;                      /* error line                        */
int32 error_column;                    /* error column                      */
int title_printed;                     /* YES if we've printed a title      */
specifier spare;                       /* return value from trap            */
int32 i,j;                             /* temporary looping variables       */
char *p, *mt;                          /* temporary looping variable        */
#if UNIX_VARARGS
char vp_pattern[20];                   /* saved vsprintf string             */
char vp_size;                          /* saved vp_size field               */
char vp_c_arg;                         /* hold character arg                */
char *vp_s_arg;                        /* hold string                       */
int vp_i_arg;                          /* hold integer arg                  */
int *vp_n_arg;                         /* hold integer pointer              */
void *vp_p_arg;                        /* hold void pointer                 */
long vp_l_arg;                         /* hold long arg                     */
double vp_f_arg;                       /* hold float arg                    */
char *vp_p, *vp_s;                     /* used to traverse buffer           */
#endif
static int abend_nested_calls=0;

   /* we don't call recursively */

#ifdef TSAFE
   if (plugin_instance==NULL) {
      if (++abend_nested_calls>3)
	   	return;
   } else {
   if (++NESTED_CALLS > 3)
      return;
   }
#else
   if (++NESTED_CALLS > 3)
      return;
#endif

   /* find the location of the current position */

   find_position(SETL_SYSTEM
                 cstack[cstack_top].cs_unittab_ptr,
                 cstack[cstack_top].cs_code_type,
                 ip,
                 ABEND__SOURCE_NAME,&error_line,&error_column);

   /* print the error message */

   sprintf(ABEND_MESSAGE,
           "\n*** Abnormal End -- source file => %s\n",
           ABEND__SOURCE_NAME);
   mt = ABEND_MESSAGE + strlen(ABEND_MESSAGE);
   sprintf(mt,
           "                    line   => %d\n",
           error_line);
   mt += strlen(mt);
   sprintf(mt,
           "                    column => %d\n\n",
           error_column);
   mt += strlen(mt);

   /*
    *  The following ugliness accommodates MIPS brain damage.  First, the
    *  stdarg.h macros in gcc don't work.  To make matters worse, the
    *  varargs.h macros don't work with the v?printf functions.  So, I
    *  have to do this futzing myself.
    */

#if UNIX_VARARGS

   va_start(arg_pointer);
   vp_s = message;
   while (*vp_s) {

      /* find the next format string */

      while (*vp_s && *vp_s != '%')  
         *mt++ = *vp_s++;

      if (!*vp_s) {
         *mt = '\0';
         break;
      }
      if (*(vp_s+1) == '%') {
         *mt++ = '%';
         vp_s += 2;
         continue;
      }
      vp_p = vp_pattern;
      *vp_p++ = *vp_s++;
      
      /* skip flags */

      while (*vp_s == '+' ||
             *vp_s == '-' ||
             *vp_s == '#' ||
             *vp_s == ' ')
         *vp_p++ = *vp_s++;

      /* skip width, precision */

      if (is_digit(*vp_s,10)) {
         while (is_digit(*vp_s,10))
            *vp_p++ = *vp_s++;
         if (*vp_s == '.') {
            *vp_p++ = *vp_s++;
            while (is_digit(*vp_s,10))
               *vp_p++ = *vp_s++;
         }
      }

      /* skip vp_size */

      if (*vp_s == 'h' ||
          *vp_s == 'l' ||
          *vp_s == 'L') {

         vp_size = *vp_s;
         *vp_p++ = *vp_s++;

      }
      else {
       
         vp_size = 0;

      }

      /* finally, we should have the type field */
     
      *vp_p = *vp_s++;
      *(vp_p+1) = '\0';
      switch (*vp_p) {

         case 'c' :

            vp_c_arg = va_arg(arg_pointer,char);
            sprintf(mt,vp_pattern,vp_c_arg);
            break;

         case 'd' :
         case 'i' :
         case 'o' :
         case 'u' :
         case 'x' :
         case 'X' :

            if (vp_size == 'l') { 
               vp_l_arg = va_arg(arg_pointer,long);
               sprintf(mt,vp_pattern,vp_l_arg);
            }
            else {
               vp_i_arg = va_arg(arg_pointer,int);
               sprintf(mt,vp_pattern,vp_i_arg);
            }
            break;

         case 'e' :
         case 'E' :
         case 'f' :
         case 'g' :
         case 'G' :
          
            vp_f_arg = va_arg(arg_pointer,double);
            sprintf(mt,vp_pattern,vp_f_arg);
            break;

         case 's' :
          
            vp_s_arg = va_arg(arg_pointer,char *);
            sprintf(mt,vp_pattern,vp_s_arg);
            break;

         case 'n' :

            vp_n_arg = va_arg(arg_pointer,int *);
            sprintf(mt,vp_pattern,vp_n_arg);
            break;

         case 'p' :

            vp_p_arg = va_arg(arg_pointer,void *);
            sprintf(mt,vp_pattern,vp_p_arg);
            break;

         default :
            strcpy(mt,vp_pattern);

      }

      /* skip past the appended string */

      while (*mt)
         mt++;

   }

   va_end(arg_pointer);

#else

   va_start(arg_pointer,message);
   vsprintf(mt, message, arg_pointer);
   mt += strlen(mt);
   va_end(arg_pointer);

#endif

   /* generate source markup, if desired */

   if (MARKUP_SOURCE) {

      /* flag location */

      fprintf(stdout,"!!! ABEND\n");
      fprintf(stdout,"!!! file \"%s\"; line \"%d\"\n",
                     ABEND__SOURCE_NAME,
                     error_line);

      /* generate source comments */

      fprintf(stdout,"--!");
      for (i = 4; i < error_column; i++)
         fprintf(stdout," ");
      fprintf(stdout,"^\n");

      fprintf(stdout,"--! *ABORT* ");
      for (p = ABEND_MESSAGE; *p; p++) {

         fprintf(stdout,"%c",*p);
         if (*p == '\n')
            fprintf(stdout,"--!         ");

      }
      fprintf(stdout,"\n");

   }

   /* print the call stack */

   title_printed = NO;
   
   if (cstack_top) /* HACK: Giubeppe & Toto 10-18-1999 */
		 for (i = cstack_top; i >= 0; i--) {

	      /* skip irrelevant stack entries */

	      if (cstack[i].cs_pc == NULL ||
	          cstack[i].cs_proc_ptr == NULL)
	         continue;

	      /* find the previous good entry */

	      for (j = i - 1;
	           j > 0 && 
	              (cstack[j].cs_pc == NULL || cstack[j].cs_proc_ptr == NULL);
	           j--);

	      /* if we haven't printed a title, do so now */

	      if (!title_printed) {

	         title_printed = YES;
	         sprintf(mt,"\n  Call Stack\n  ----------\n\n");
	         mt += strlen(mt);
	         sprintf(mt,"  Line  Column  File\n");
	         mt += strlen(mt);
	         sprintf(mt,"  ----  ------  --------------------------------\n");
	         mt += strlen(mt);

	      }

	      /* find the position in the source file */

	      find_position(SETL_SYSTEM
	                    cstack[j].cs_unittab_ptr,
	                    cstack[j].cs_code_type,
	                    cstack[i].cs_pc - 1,
	                    ABEND__SOURCE_NAME,&error_line,&error_column);

	      /* print the location */

	      sprintf(mt, "%6ld  %6ld  %s\n",error_line,error_column,ABEND__SOURCE_NAME);
	      mt += strlen(mt);

	   }

#ifdef WINX
   MessageBox(NULL, ABEND_MESSAGE, "SETL2 Abort", MB_OK);
#else
#ifdef PANEL
   MessageBox(NULL, ABEND_MESSAGE, "SETL2 Abort", MB_OK);
#else
   if (VERBOSE_MODE>0)
      fprintf(stderr,"%s\n\n",ABEND_MESSAGE);
#endif
#endif

   /* make sure our callback is a procedure */
	 
   if (spec_abendtrap->sp_form != ft_proc) {
#ifdef PLUGIN
			 hard_stop=1;
	 		 if (abend_initialized) {
	       longjmp(abend_env,1);
          }
#else
      exit(ABEND_EXIT);
#endif
 
   }
   /* call the abend trap */

   spare.sp_form = ft_omega;
   call_procedure(SETL_SYSTEM &spare,
                  spec_abendtrap,
                  NULL,
                  0L,YES,NO,0);
   unmark_specifier(&spare);
#ifdef PLUGIN
	 hard_stop=1;
   if (abend_initialized) {
	       longjmp(abend_env,1);
   }
#else
	    exit(ABEND_EXIT);
#endif

}

/*\
 *  \function{find\_position()}
 *
 *  This function finds the file position corresponding to a passed
 *  location.  We search through the library each time, and don't worry
 *  about the cost.  We're only going to do this in a emergency
 *  situation, when the program is aborting.
\*/

static void find_position(
   SETL_SYSTEM_PROTO
   unittab_ptr_type unittab_ptr,       /* unit containing code              */
   int code_type,                      /* initialization or body code       */
   instruction *pc,                    /* instruction to find               */
   char *source_name,                  /* returned file name                */
   int32 *line,                        /* returned line number              */
   int32 *column)                      /* returned column number            */

{
char *unit_name;                       /* text unit name                    */
libunit_ptr_type libunit_ptr;          /* library unit pointer              */
libstr_ptr_type libstr_ptr;            /* library stream pointer            */
unit_control_record unit_control;      /* unit control record               */
pcode_record pcode;                    /* pseudo code record                */
int32 i;                               /* temporary looping variable        */

   /* find the unit in the library */

   for (unit_name = unittab_ptr->ut_name;
        *unit_name && *unit_name != ':';
        unit_name++);
   if (!*unit_name)
      unit_name = unittab_ptr->ut_name;
   else
      unit_name++;
   libunit_ptr = open_libunit(SETL_SYSTEM unit_name,NULL,LIB_READ_UNIT);

#ifdef TRAPS

   if (libunit_ptr == NULL)
      trap(__FILE__,__LINE__,msg_abend_failed);

#endif

   /* load the unit control record */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_CONTROL_STREAM);
   read_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
               sizeof(unit_control_record));
   close_libstr(SETL_SYSTEM libstr_ptr);

   /* now we can figure out the file name */

#ifdef DEBUG

   if (code_type == EX_INIT_CODE &&
       (((pc - unittab_ptr->ut_init_code) / (1 + EX_DEBUG)) <=
       unit_control.uc_sipcode_count)) {

#else

   if (code_type == EX_INIT_CODE &&
       (pc - unittab_ptr->ut_init_code) <=
       unit_control.uc_sipcode_count) {

#endif

      strcpy(source_name,unit_control.uc_spec_source_name);

   }
   else {

      strcpy(source_name,unit_control.uc_body_source_name);

   }

   /* read through the instructions until we find the target */

   if (code_type == EX_BODY_CODE) {

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_PCODE_STREAM);

#if DEBUG

      for (i = 0;
           i <= (pc - unittab_ptr->ut_body_code) / (1 + EX_DEBUG);
           i++) {

#else

      for (i = 0;
           i <= pc - unittab_ptr->ut_body_code;
           i++) {

#endif

         read_libstr(SETL_SYSTEM libstr_ptr,(char *)&pcode,sizeof(pcode_record));
         if (pcode.pr_file_pos.fp_line > 0) {

            *line = pcode.pr_file_pos.fp_line;
            *column = pcode.pr_file_pos.fp_column;

         }
      }
   }
   else {

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INIT_STREAM);
  
#if DEBUG

      for (i = 0;
           i <= (pc - unittab_ptr->ut_init_code) / (1 + EX_DEBUG);
           i++) {

#else

      for (i = 0;
           i <= pc - unittab_ptr->ut_init_code;
           i++) {

#endif

         read_libstr(SETL_SYSTEM libstr_ptr,(char *)&pcode,sizeof(pcode_record));
         if (pcode.pr_file_pos.fp_line > 0) {

            *line = pcode.pr_file_pos.fp_line;
            *column = pcode.pr_file_pos.fp_column;

         }
      }
   }
}

