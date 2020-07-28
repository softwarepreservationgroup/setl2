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
 *  \package{Main}
 *
 *  This file contains the main function of the SETL2 interpreter.  We
 *  initialize all the tables and process command line options.  All
 *  argments before the program name are assumed to be for the
 *  interpreter itself.  Others are gathered into a tuple for use by the
 *  SETL2 program.
\*/

#include "system.h"                    /* SETL2 system constants            */

#ifdef PLUGIN
#define fprintf plugin_fprintf
#define printf plugin_printf
#endif


#ifdef MACINTOSH
#include <Types.h>
#include <Files.h>
#include <PPCToolBox.h>
#include <CodeFragments.h>
#include <TextUtils.h>
#endif /* MACINTOSH */

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif


#ifdef WIN32
#include <windows.h>
#endif

/* standard C header files */
#include <ctype.h>                     /* character macros                  */
#ifdef HAVE_SIGNAL_H
#include <signal.h>                    /* signal macros                     */
#endif

#ifdef __MWERKS__ /* GDM */
#include <Console.h>
#endif


/* SETL2 system header files */

#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "cmdline.h"                   /* command line argument string      */
#include "form.h"                      /* form codes                        */
#include "filename.h"                  /* file name utilities               */
#include "libman.h"                    /* library manager                   */
#include "builtins.h"                  /* built-in symbols                  */
#include "unittab.h"                   /* unit table                        */
#include "loadunit.h"                  /* unit loader                       */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_reals.h"                   /* real numbers                      */
#include "x_strngs.h"                  /* strings                           */
#include "tuples.h"                    /* tuples                            */
#include "slots.h"                     /* procedures                        */
#include "pcode.h"                     /* pseudo code                       */

unittab_ptr_type head_unit_ptr=NULL;        /* program unit pointer              */

/* forward & external declarations */

static char *next_arg(SETL_SYSTEM_PROTO int, char **); 
                                       /* get next program argument         */

#ifdef PLUGIN
void compiler_cleanup(SETL_SYSTEM_PROTO_VOID);
#endif

void runtime_cleanup(SETL_SYSTEM_PROTO_VOID);
void profiler_dump(SETL_SYSTEM_PROTO_VOID);
void setl2_reset_callback(SETL_SYSTEM_PROTO_VOID);


/*\
 *  \function{main()}
 *
 *  This function executes the interpreter.  First we process command
 *  line options, we build a tuple out of options following the file
 *  name, we load the program, and we execute it.
\*/


#ifdef PLUGIN
#ifdef TSAFE
         int plugin_main(void *instance_pass, char *fileName)
#else
         int plugin_main(char *fileName)
#endif
#else
void main(
   int argc,                           /* number of command line arguments  */
   char **argv)                        /* command line argument vector      */
#endif
{
char *arg;                             /* current argument pointer          */
char program[MAX_UNIT_NAME + 1];       /* program to be executed            */
tuple_h_ptr_type tuple_root, tuple_work_hdr, new_tuple_hdr;
                                       /* tuple header pointers             */
tuple_c_ptr_type tuple_cell;           /* tuple cell pointer                */
int tuple_index, tuple_height;         /* used to descend header trees      */
int32 tuple_length;                    /* current tuple length              */
int32 expansion_trigger;               /* size which triggers header        */
                                       /* expansion                         */
string_h_ptr_type target_hdr;          /* target string                     */
string_c_ptr_type target_cell;         /* target string cell                */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
unittab_ptr_type unit_ptr;             /* program unit pointer              */
char *p,*q,*s;                         /* temporary looping variable        */
int i;                                 /* temporary looping variable        */
struct plugin_item *plugin_instance;


#ifndef PLUGIN

#ifdef TSAFE
   plugin_instance=Setl_Initialize();
#else
   Setl_Initialize();
#endif

#else

#ifdef TSAFE
   plugin_instance=(struct plugin_item *)instance_pass;
#else
   Setl_Initialize();
#endif

#endif

#ifndef PLUGIN
#ifdef MACINTOSH

argc == ccommand(&argv);

#endif /* MACINTOSH */
#endif /* PLUGIN */

   /* set ^C trap */

#ifdef HAVE_SIGNAL_H
   if (signal(SIGINT,user_interrupt) == SIG_ERR)
      giveup(SETL_SYSTEM msg_trap_user);
#endif

#if UNIX | VMS
#ifdef DEBUG

#ifdef HAVE_SIGNAL_H
   if (signal(SIGSEGV,i_segment_error) == SIG_ERR)
      giveup(SETL_SYSTEM msg_trap_segment);
#endif

#endif
#endif

   /*
    *  Before examining the command line arguments, we set library
    *  file names from the environment.
    */

   if ((p = getenv(LIB_KEY)) != NULL)
      DEFAULT_LIBRARY = p;

   if ((p = getenv(LIBPATH_KEY)) != NULL)
      LIBRARY_PATH = p;

   strcpy(program,fileName);

   for (p = program; *p; p++) {
      if (islower(*p))
         *p = toupper(*p);
   }

   /* initialize tables */

   Setl_InitInterpreter(SETL_SYSTEM_VOID);
 //   head_unittab=NULL; 
 // last_unittab=NULL; 
   /* build the library search path */
   /* The libraries are separated by commas */ 

   s=p=DEFAULT_LIBRARY;

#ifdef DYNAMIC_COMP
   i=YES; /* Make sure we can write onto this library... */
#else 
   i=NO;
#endif
   while (s!=NULL) {
      while ((*p!=0)&&(*p!=',')) p++;
         if ((p-s)<=0) { 
            s=NULL;
         } else {
           q=(char *)malloc((size_t)(p-s+ 1));
           if (q == NULL)
              giveup(SETL_SYSTEM msg_malloc_error);
           strncpy(q,s,(p-s));
           q[p-s]='\0';
           add_lib_file(SETL_SYSTEM q,i);
           i=NO;    
           free(q);
           if (*p==0) s=NULL; 
           else {
             s=p+1;
             p=s;
           }
	 }
   }


   add_lib_path(SETL_SYSTEM LIBRARY_PATH);

   /*
    *  We build up the command line tuple before loading the program, in
    *  case the initialization code uses the command line tuple.
    */

   /* initialize a tuple to be returned */

   tuple_root=new_tuple(SETL_SYSTEM_VOID);

   expansion_trigger = TUP_HEADER_SIZE;

   /* loop through remaining arguments */

   tuple_length = 0;
#ifndef PLUGIN
#ifndef __MWERKS__

   while ((arg = next_arg(SETL_SYSTEM argc,argv)) != NULL) {

      /* make a target string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = strlen(arg);
      target_hdr->s_head = target_hdr->s_tail = NULL;
      target_char_ptr = target_char_end = NULL;

      /* copy the argument to the string */

      for (p = arg; *p; p++) {

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

      /*
       *  Now we have to insert the string we just created into the
       *  command line tuple.
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

#endif /* __MWERKS__ */
#endif /* PLUGIN */

   /* set the command line tuple to the tuple we just created */

   tuple_root->t_ntype.t_root.t_length = tuple_length;
   spec_cline->sp_form = ft_tuple;
   spec_cline->sp_val.sp_tuple_ptr = tuple_root;

   /* now that the command line tuple is built, we can load the program */

   TRACING_ON = EX_DEBUG;
   head_unit_ptr=NULL;
   unit_ptr = load_unit(SETL_SYSTEM program, NULL, NULL); 
   if (unit_ptr == NULL) {
      runtime_cleanup(SETL_SYSTEM_VOID);
      giveup(SETL_SYSTEM msg_program_not_found,program);
   }
   head_unit_ptr=unit_ptr;
/*
   unit_ptr = load_eval_unit(SETL_SYSTEM program, NULL, NULL);
   if (unit_ptr == NULL) {
      return -1;
   }
   NESTED_CALLS=0;
   strcpy(ABEND_MESSAGE,"");
   
   execute_setup(SETL_SYSTEM unit_ptr, EX_BODY_CODE); 
   execute_go(SETL_SYSTEM YES);
   return 0;
*/

   NESTED_CALLS=0;
   cstack_top = -1;
   critical_section = 0;
   opcodes_until_switch = 2000;
   pstack_top = -1;                 /* top of program stack              */
   pstack_max = 0;                  /* size of program stack             */
   cstack_top = -1;                 /* top of call stack                 */
   cstack_max = 0;  

   execute_setup(SETL_SYSTEM unit_ptr, EX_BODY_CODE);
   /* interpret the program */

#ifdef PLUGIN
   return 0;
#else
   execute_go(SETL_SYSTEM YES);
#endif
	
   runtime_cleanup(SETL_SYSTEM_VOID);
   profiler_dump(SETL_SYSTEM_VOID);
#ifndef PLUGIN
   exit(SUCCESS_EXIT);
#endif

}

void Setl_SetCommandLine(SETL_SYSTEM_PROTO int argc,int optind,char **argv)
{
tuple_h_ptr_type tuple_root, tuple_work_hdr, new_tuple_hdr;
                                       /* tuple header pointers             */
tuple_c_ptr_type tuple_cell;           /* tuple cell pointer                */
int tuple_index, tuple_height;         /* used to descend header trees      */
int32 tuple_length;                    /* current tuple length              */
int32 expansion_trigger;               /* size which triggers header        */
                                       /* expansion                         */
string_h_ptr_type target_hdr;          /* target string                     */
string_c_ptr_type target_cell;         /* target string cell                */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
char *arg,*p;
int i;

   /*
    *  We build up the command line tuple before loading the program, in
    *  case the initialization code uses the command line tuple.
    */

   /* initialize a tuple to be returned */

   tuple_root=new_tuple(SETL_SYSTEM_VOID);

   expansion_trigger = TUP_HEADER_SIZE;

   /* loop through remaining arguments */

   tuple_length = 0;


   while (optind<argc) {

	  arg=argv[optind++];

      /* make a target string */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = strlen(arg);
      target_hdr->s_head = target_hdr->s_tail = NULL;
      target_char_ptr = target_char_end = NULL;

      /* copy the argument to the string */

      for (p = arg; *p; p++) {

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

      /*
       *  Now we have to insert the string we just created into the
       *  command line tuple.
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


   /* set the command line tuple to the tuple we just created */

   tuple_root->t_ntype.t_root.t_length = tuple_length;
   spec_cline->sp_form = ft_tuple;
   spec_cline->sp_val.sp_tuple_ptr = tuple_root;

}


void runtime_cleanup(SETL_SYSTEM_PROTO_VOID) 
{
unittab_ptr_type u;

#ifdef WIN32
void *psymb;
#endif

#if HAVE_DLFCN_H

void *psymb;
#endif

#ifdef MACINTOSH
OSErr myErr;
FSSpec myFSSpec;
Ptr myMainAddr;
Str255 myErrName;
Str63 fragName;
Str255 myName;
long myCount;
CFragSymbolClass myClass;
Ptr myAddr;
#endif
char key[1024];


                 
   for (u=head_unit_ptr;u!=NULL;
             u=u->ut_next) {
        //fprintf(stderr,"*** Unloading Unit %s ***\n",
        //       u->ut_name);
                 
      if (u->ut_type==NATIVE_UNIT) {
      //   fprintf(stderr," --> IS A NATIVE UNIT!\n");
         
         sprintf(key,"%s%s",u->ut_name,"__END");
  
#ifdef MACINTOSH
           myErr = FindSymbol((CFragConnectionID)u->ut_native_code,
                            c2pstr(key),
                            &myAddr, &myClass);
 
          if (!myErr)
            (*(int32(*)(SETL_SYSTEM_PROTO_VOID))myAddr)(SETL_SYSTEM_VOID);
            
        // if (u->ut_native_code)	{
        //    myErr =  CloseConnection((CFragConnectionID)u->ut_native_code);
        //    printf("Closed connection %d  (%d)\n",(int)((CFragConnectionID)u->ut_native_code),myErr);
        //  }
	
#endif

#if HAVE_DLFCN_H
         psymb=dlsym(u->ut_native_code,key);
         if (psymb!=NULL)
			  (*(int32(*)(SETL_SYSTEM_PROTO_VOID))psymb)(SETL_SYSTEM_VOID);
           
#endif

#ifdef WIN32
	     psymb = GetProcAddress((HINSTANCE)(u->ut_native_code), key);
	     if (psymb!=NULL)
			 (*(int32(*)(SETL_SYSTEM_PROTO_VOID))psymb)(SETL_SYSTEM_VOID);
            
		 
#endif


       }
                 
   }
   /* close the library and exit */


   close_io(SETL_SYSTEM_VOID);
   close_lib(SETL_SYSTEM_VOID);
   
#ifdef PLUGIN
   compiler_cleanup(SETL_SYSTEM_VOID);
#endif

  head_unit_ptr=NULL;
 
  setl2_reset_callback(SETL_SYSTEM_VOID);

}

void profiler_dump(SETL_SYSTEM_PROTO_VOID)
{
long l;                                /* temporary looping variable        */
int i;                                 /* temporary looping variable        */


#ifdef DEBUG
   if (PROF_DEBUG) {
      fprintf(DEBUG_FILE,"==================== SETL2 PROFILER ====================\n");
      for (last_unittab=head_unittab;last_unittab!=NULL;
             last_unittab=last_unittab->ut_next) {
         fprintf(DEBUG_FILE,"\n*** Profiling for UNIT %s ***\n\n",
                 last_unittab->ut_name);
#ifdef HAVE_GETRUSAGE
         fprintf(DEBUG_FILE,  "   Line      Opcodes    Copies      Time (s)  (us)     Time (s)  (us)\n");
         fprintf(DEBUG_FILE,  "---------------------------------- ------------------ ------------------\n");
#else
         fprintf(DEBUG_FILE,  "   Line      Opcodes    Copies \n");
         fprintf(DEBUG_FILE,  "----------------------------------\n");
#endif
         profi=last_unittab->ut_prof_table;

           for (l=0;l<=last_unittab->ut_nlines;l++) {
              if (profi->count!=0) {
#ifdef HAVE_GETRUSAGE
                 fprintf(DEBUG_FILE,  " %9ld  %9ld  %9ld    %6ld.%6ld      %6ld.%6ld\n",l,profi->count,
                       profi->copies,(long)profi->time.tv_sec,(long)profi->time.tv_usec,
                       (long)profi->timec.tv_sec,(long)profi->timec.tv_usec);
#else
                 fprintf(DEBUG_FILE,  " %9ld  %9ld  %9ld\n",l,profi->count,
                       profi->copies);
#endif
              }
              profi++;
           }
#ifdef HAVE_GETRUSAGE
         fprintf(DEBUG_FILE,  "---------------------------------- ------------------ ------------------\n");
#else
         fprintf(DEBUG_FILE,  "----------------------------------\n");
#endif
      }
      fprintf(DEBUG_FILE,"\n=================== EXECUTION SUMMARY ==================\n");

      for (i=0;i<=pcode_length;i++) {
         if (pcode_operations[i]!=0) {
            fprintf(DEBUG_FILE,"PCODE => %-13s Operations: %9ld Copies: %9ld\n",
                 pcode_desc[i],
                 pcode_operations[i],
                 copy_operations[i]);
            fflush(DEBUG_FILE);
         }

      }

   }
   if (DEBUG_FILE!=stdout) fclose(DEBUG_FILE);
  
#endif

}

/*\
 *  \function{next\_arg()}
 *
 *  This function is a command line argument scanner.  It is a little
 *  more robust than would appear necessary, but it was written to work
 *  on both the Unix and MS-DOS operating systems.  In a Unix system, we
 *  would ordinarily not have to worry about default options from the
 *  environment, since the user would probably use an alias to accomplish
 *  the same thing.  Environment options are a convenience in MS-DOS,
 *  however.
 *
 *  The first time this function is called, we use {\tt malloc()} to
 *  allocate a buffer for argument strings, and load the default
 *  arguments from the environment.  Then we scan the command line
 *  arguments.  We recognize the following types of tokens:
 *
 *  \begin{itemize}
 *  \item
 *  Space-delimited tokens.  These should be the most frequent.  Notice
 *  that when space delimited tokens are in the command line rather than
 *  the environment, we receive them already tokenized.
 *  \item
 *  Option definitions.  These all start with a dash, and are two
 *  characters long.  Subsequent qualifiers (file names, for example)
 *  should not start with a dash.
 *  \item
 *  Quoted strings.  Both Unix and MS-DOS handle quoted strings for us,
 *  except in one case --- when the options come from an environment
 *  string.  We only check for quoted strings when the source is an
 *  environment string.
 *  \end{itemize}
\*/

static char *next_arg(SETL_SYSTEM_PROTO
                      int argc,        /* number of command line arguments  */
                      char **argv)     /* command line argument vector      */

{
#ifdef TSAFE

#define FIRST_TIME plugin_instance->first_time
#define ARG_BUFFER plugin_instance->arg_buffer
#define ARG_PTR plugin_instance->arg_ptr
#define CARG_NUM plugin_instance->carg_num

#else

#define FIRST_TIME first_time
#define ARG_BUFFER arg_buffer
#define ARG_PTR arg_ptr
#define CARG_NUM carg_num

static int first_time = YES;           /* YES only on the first call        */
static char *arg_buffer = NULL;        /* buffer for argument strings       */
static char *arg_ptr;                  /* next position in active argument  */
static int carg_num = 0;               /* command line argument number      */
#endif

char *p;                               /* temporary looping variable        */

   /* the first time we're called ... */

   if (FIRST_TIME) {

      FIRST_TIME = NO;

      /* allocate a buffer for arguments we find */

      ARG_BUFFER = (char *)malloc((size_t)(PATH_LENGTH + 1));
      if (ARG_BUFFER == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);

      /* set the argument pointer to the environment string */

      if ((ARG_PTR = cmdline()) == NULL) {
         if (argc == 1)
            return NULL;
         CARG_NUM = 1;
         ARG_PTR = argv[1];
      }
   }

   /* loop until we explicitly return */

   for (;;) {

      /* skip white space */

      while (*ARG_PTR && *ARG_PTR <= ' ')
         ARG_PTR++;

      /* if we've finished a string, get the next from the command line */

      if (*ARG_PTR == '\0') {
         CARG_NUM++;

         /*
          *  if there are no more command line arguments, free the
          *  buffer we allocated (if necessary) and return NULL
          */

         if (CARG_NUM == argc) {
            if (ARG_BUFFER != NULL) {
               free(ARG_BUFFER);
               ARG_BUFFER = NULL;
            }
            return NULL;
         }

         /* otherwise, start scanning the next argument */

         ARG_PTR = argv[CARG_NUM];
         continue;
      }

      /* pick off quoted strings from environment */

      if (CARG_NUM == 0 && *ARG_PTR == '"') {
         ARG_PTR++;
         for (p = ARG_BUFFER;
              p - ARG_BUFFER <= PATH_LENGTH && *ARG_PTR && *ARG_PTR != '"';
              *p++ = *ARG_PTR++) {
            if (*ARG_PTR == '\\' && *(ARG_PTR + 1)) {
               ARG_PTR++;
               *p++ = *ARG_PTR++;
            }
         }
         if (p - ARG_BUFFER > PATH_LENGTH)
            giveup(SETL_SYSTEM msg_opt_string_too_long,
                   PATH_LENGTH);
         if (!*ARG_PTR)
            giveup(SETL_SYSTEM "Unmatched '\"' in options");
         ARG_PTR++;
         *p = '\0';
         return ARG_BUFFER;
      }

      /* pick off options */

      if (*ARG_PTR == '-' && *(ARG_PTR + 1)) {
         strncpy(ARG_BUFFER,ARG_PTR,2);
         ARG_BUFFER[2] = '\0';
         ARG_PTR += 2;
         return ARG_BUFFER;
      }

      /*
       *  if we're not scanning the environment string, return the rest of
       *  the string
       */

      if (CARG_NUM != 0) {
         for (p = ARG_BUFFER; *ARG_PTR; *p++ = *ARG_PTR++);
         *p = '\0';
         return ARG_BUFFER;
      }

      /* find space-delimited tokens in the environment */

      for (p = ARG_BUFFER;
           p - ARG_BUFFER <= PATH_LENGTH &&
              *ARG_PTR && *ARG_PTR != ' ' && *ARG_PTR != '\t';
           *p++ = *ARG_PTR++);
      if (p - ARG_BUFFER > PATH_LENGTH)
         giveup(SETL_SYSTEM msg_opt_string_too_long,
                PATH_LENGTH);
      *p = '\0';
      return ARG_BUFFER;

   }
}

