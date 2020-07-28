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
 *  This is the main function for the SETL2 compiler. It is responsible for
 *  most of the interaction with the user and the environment.  It
 *  performs the following:
 *
 *  \begin{enumerate}
 *  \item
 *  It parses the command line, setting options.
 *  \item
 *  It expands file specifications into lists of file names.
 *  \item
 *  It calls the parser and code generators for each source file
 *  \end{enumerate}
\*/


/* standard C header files */
#include <stdio.h>
#if UNIX
#include <sys/types.h>                 /* file types                        */
#include <sys/stat.h>                  /* file status                       */
#endif
#ifndef VMS
#include <fcntl.h>                     /* file functions                    */
#endif
#include <ctype.h>                     /* character macros                  */
#ifdef HAVE_SIGNAL_H
#include <signal.h>                    /* signal macros                     */
#endif


#include "system.h"                    /* SETL2 system constants            */

/* SETL2 system header files */


#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "filename.h"                  /* file name utilities               */
#include "libman.h"                    /* library manager                   */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "ast.h"                       /* abstract syntax tree              */
#include "quads.h"                     /* quadruples                        */
#include "c_integers.h"                  /* integer literals                  */
#include "c_reals.h"                     /* real literals                     */
#include "c_strngs.h"                    /* string literals                   */
#include "import.h"                    /* imported packages and classes     */
#include "parse.h"                     /* parser                            */
#include "genquads.h"                  /* quadruple generator               */
#include "geniter.h"                   /* iterator code generator           */
#include "optimize.h"                  /* optimizer                         */
#include "genpcode.h"                  /* pseudo-code generator             */
#include "listing.h"                   /* source and error listings         */
#include "builtins.h"                  /* built-in symbols		    */ 

/* package-global data */
#ifdef PLUGIN
extern char *plugin_lib_file;
#endif

#ifdef PLUGIN
extern char *setl_lib_file;
#define printf plugin_printf
#endif

static int file_count;                 /* number of source files processed  */
#ifdef DYNAMIC_COMP 
#ifndef TSAFE
extern int verbose_mode;
#endif
#endif

/* forward declarations */


void 
compiler_init(SETL_SYSTEM_PROTO_VOID) {

   DEBUG_FILE = stdout;
#ifdef PLUGIN
   DEFAULT_LIBRARY = setl_lib_file;  /* default library name              */
#else
   DEFAULT_LIBRARY = "setl2.lib";      /* default library name              */
#endif
   DEBUG_FILE = stdout;
   LIBRARY_PATH = "";                  /* library search path               */
   MARKUP_SOURCE = 0;

   VERBOSE_MODE = 1;
   IMPLICIT_DECLS = 1;                 /* do implicit variable declarations */
   GENERATE_LISTING = 0;               /* generate a program listing        */
   SAFETY_CHECK = 1;                   /* check for duplicate units before  */
				       /* storing                           */
   USE_INTERMEDIATE_FILES = 0;         /* use intermediate files for ast's  */
   TAB_WIDTH = 8;                      /* tab width                         */
   SOURCE_FILE = NULL;                 /* source file handle                */
   I1_FILE = NULL;                     /* intermediate library file         */
   I2_FILE = NULL;                     /* intermediate AST file             */
   DEFAULT_LIBFILE = NULL;
				       /* default library                   */
   UNIT_ERROR_COUNT = 0;               /* errors in compilation unit        */
   FILE_ERROR_COUNT = 0;               /* errors in source file             */
   TOTAL_ERROR_COUNT = 0;              /* total error count                 */
   TOTAL_WARNING_COUNT = 0;            /* warnings in compilation unit      */
   FILE_WARNING_COUNT = 0;             /* warnings in source file           */
   TOTAL_WARNING_COUNT = 0;            /* total warning count               */
   TOTAL_GLOBAL_SYMBOLS = 0;           /* Total number of global symbols    */
				       /* defined in the EVALs              */
   COMPILING_EVAL = NO;                /* YES if we are compiling a fragment*/
   NUMEVAL=0;                          /* Incremented when a global proc.   */
				       /* is defined in an EVAL             */
   DEFINING_PROC = NO;                 /* YES if we defined a global proc.  */
   OPTIMIZE_OF = NO;
   OPTIMIZE_ASSOP = NO;
   COMPILER_OPTIONS = 0;               /* Verbose modes                     */

#ifdef DEBUG

   PRS_DEBUG = 0;                      /* debug parser flag                 */
   LEX_DEBUG = 0;                      /* debug lexical analyzer flag       */
   SYM_DEBUG = 0;                      /* debug symbol table flag           */
   AST_DEBUG = 0;                      /* debug symbol table flag           */
   PROCTAB_DEBUG = 0;                  /* debug procedure table             */
   QUADS_DEBUG = 0;                    /* debug quadruple lists             */
   CODE_DEBUG = 0;                     /* debug code generator              */

#endif

#ifdef DYNAMIC_COMP
   GLOBAL_HEAD=NULL;
#endif

COMPILING_EVAL = NO;

}

setl_pack_program(SETL_SYSTEM_PROTO
				  char *newcode,char *buffer) {

   if (NUMEVAL==0){

      sprintf(newcode,"package eval_vars; var eval_0000; end eval_vars;program eval_prog%ld; use eval_vars; \n%s\n end eval_prog%ld;",NUMEVAL,buffer,NUMEVAL);

   } else {

      sprintf(newcode,"program eval_prog%ld; use eval_vars;\n%s\n end eval_prog%ld;",NUMEVAL,buffer,NUMEVAL);

   }
}

setl_incr_numeval(SETL_SYSTEM_PROTO_VOID) {

  NUMEVAL++;

}

int setl_get_numeval(SETL_SYSTEM_PROTO_VOID) {

  return NUMEVAL;

}


/*\
 *
 *  \function{main()}
 *
 *  This function reads arguments from the environment and the command
 *  line.  It sets various options (mostly flags), and compiles the source
 *  file names it finds.
 *
\*/


char setl2_program_name[64];

#ifdef DYNAMIC_COMP
/* 
 * This function is defined in dynamic compilation mode, when the compiler
 * is linked with the interpreter.
 *
 */
int compile_fragment(
   SETL_SYSTEM_PROTO
   char *program_source,
   int  compile_flag)
{

char *temp_path = "";                  /* path for temporary files          */
char file_path[PATH_LENGTH + 1];       /* path name of source files         */
char *unit_file_name;                  /* file name for file unit           */
char *p,*q,*s;                         /* temporary looping variable        */
proctab_ptr_type proctab_ptr;          /* used to loop over compilation     */
int save_global_symbols; 
global_ptr_type save_global_head,temp_global_head;
int i;

   /* initialize file and error counts */

   DEFINING_PROC = NO;
   file_count = 0;
   UNIT_ERROR_COUNT = 0;               /* errors in compilation unit        */
   FILE_ERROR_COUNT = 0;               /* errors in source file             */
   FILE_WARNING_COUNT = 0;             /* warnings in source file           */
   TOTAL_WARNING_COUNT = 0;            /* total warning count               */
   TOTAL_GLOBAL_SYMBOLS = 0;           /* Total number of global symbols    */

   if (compile_flag==YES) COMPILING_EVAL = compile_flag;
   else COMPILING_EVAL=0;
   SAFETY_CHECK = 0;
 
   strcpy(setl2_program_name,"");

   /*
    *  Before examining the command line arguments, we set library
    *  file names from the environment.
    */

   if ((p = getenv(LIB_KEY)) != NULL)
      DEFAULT_LIBRARY = p;

   if ((p = getenv(LIBPATH_KEY)) != NULL)
      LIBRARY_PATH = p;
   /*
    *  We can use the same temporary file names for the entire run.  Here
    *  we find unused file names for each of the temporary files we will
    *  need, and make a working library out of one of them.
    */

   if ((p = getenv(TEMP_PATH_KEY)) != NULL)
      temp_path = p;


   if (compile_flag!=YES) {
     get_tempname(SETL_SYSTEM temp_path,I1_FNAME);
      get_tempname(SETL_SYSTEM temp_path,I2_FNAME);


      create_lib_file(SETL_SYSTEM I2_FNAME);
/*      strcpy(C_SOURCE_NAME,"<plugin_fragment>"); */

   } else {

      if (I2_FILE==NULL) {
         strcpy(I2_FNAME, MEM_LIB_NAME); 
         create_lib_file(SETL_SYSTEM I2_FNAME);
      }
      strcpy(C_SOURCE_NAME,"<eval>");
   }

   /* initialize the error counts */

   FILE_ERROR_COUNT = 0;
   FILE_WARNING_COUNT = 0;

   /* initialize tables */

   init_import();
   init_integers();
   init_compiler_reals(SETL_SYSTEM_VOID);
   init_strings(SETL_SYSTEM_VOID);
   init_namtab(SETL_SYSTEM_VOID);
   init_symtab();
   init_ast();
   init_quads();
   init_iter();
   init_proctab(SETL_SYSTEM_VOID);
   free_err_table(SETL_SYSTEM_VOID);


   /* parse the source file */
   USE_INTERMEDIATE_FILES = 0;
   PROGRAM_FRAGMENT=program_source;
   parsefile(SETL_SYSTEM_VOID);


   /* open the libraries */

   if (compile_flag!=1) {
        if (compile_flag==0) open_lib();
	I2_FILE = add_lib_file(SETL_SYSTEM I2_FNAME,YES);

   s=p=DEFAULT_LIBRARY;
   i=YES;
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
           if (i==YES)  {
                DEFAULT_LIBFILE = add_lib_file(SETL_SYSTEM q,i);
           } else 
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





   } else { 
      if (I2_FILE==NULL) { 
/* SPAM */
         TOTAL_GLOBAL_SYMBOLS = 0;
         I2_FILE = add_lib_file(SETL_SYSTEM I2_FNAME,YES);
         GLOBAL_HEAD=NULL;
         NUMEVAL = 0;
      }
      save_global_symbols = TOTAL_GLOBAL_SYMBOLS;

      save_global_head = GLOBAL_HEAD;
      while ( save_global_head!=NULL ) {
    	   save_global_head->gl_present=NO;	
    	   save_global_head=save_global_head->gl_next_ptr;	
         }
      save_global_head = GLOBAL_HEAD;
   }

   /* generate code, write to work library */
   

   for (proctab_ptr = predef_proctab_ptr->pr_child;
        proctab_ptr != NULL;
        proctab_ptr = proctab_ptr->pr_next) {

      /* reset error counts */

      UNIT_ERROR_COUNT = 0;
      UNIT_WARNING_COUNT = 0;

      /* generate quadruples */

      if (FILE_ERROR_COUNT == 0)
         gen_quads(SETL_SYSTEM proctab_ptr);

      if (FILE_ERROR_COUNT == 0 && UNIT_ERROR_COUNT == 0)
         optimize(SETL_SYSTEM proctab_ptr);

      /* if we still have no errors, update the work library */

      if (FILE_ERROR_COUNT == 0 && UNIT_ERROR_COUNT == 0)
         gen_pcode(SETL_SYSTEM proctab_ptr);

      /* update file error counts */

	if (proctab_ptr->pr_type==pr_program)
		   strcpy(setl2_program_name,(proctab_ptr->pr_namtab_ptr)->nt_name);


      FILE_ERROR_COUNT += UNIT_ERROR_COUNT;
      FILE_WARNING_COUNT += UNIT_WARNING_COUNT;

   }
   /* print summary statistics */

   if (compile_flag==1) {
      if (FILE_ERROR_COUNT == 0) {
         return (SUCCESS_EXIT);
     }
     else {
        TOTAL_GLOBAL_SYMBOLS = save_global_symbols;
        GLOBAL_HEAD = save_global_head;
        return (COMPILE_ERROR_EXIT);
     }
   }

   /* Compiling a standard fragment */


   if (FILE_ERROR_COUNT == 0) {
	   

    if ((compile_flag==0)&&(VERBOSE_MODE==2)) {
      printf("Compiling file => %s\n\n",C_SOURCE_NAME);

      print_errors(SETL_SYSTEM_VOID); 
      printf("   %3d errors\n   %3d warnings\n\n",
             FILE_ERROR_COUNT,FILE_WARNING_COUNT);
    }

      for (proctab_ptr = predef_proctab_ptr->pr_child;
           proctab_ptr != NULL;
           proctab_ptr = proctab_ptr->pr_next) {

         copy_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                      I2_FILE,DEFAULT_LIBFILE);

      }
   } else {

    if ((compile_flag==0)&&(VERBOSE_MODE>0)) {
      printf("Error Compiling file => %s\n\n",C_SOURCE_NAME);
      print_errors(SETL_SYSTEM_VOID); 
      printf("   %3d errors\n   %3d warnings\n\n",
             FILE_ERROR_COUNT,FILE_WARNING_COUNT);
    }

   }


   if (compile_flag==0) close_lib(SETL_SYSTEM_VOID);
   I2_FILE = NULL;
   if (os_access(I2_FNAME,0) == 0)
      unlink(I2_FNAME);

   if (FILE_ERROR_COUNT == 0) {
         return (SUCCESS_EXIT);
   } else return (COMPILE_ERROR_EXIT);

}

#endif  /* DYNAMIC_COMP */

#ifdef PLUGIN
int setl_error_count(SETL_SYSTEM_PROTO_VOID) {
  return FILE_ERROR_COUNT;
}

int setl_warning_count(SETL_SYSTEM_PROTO_VOID) {
  return FILE_WARNING_COUNT;
}
 
void compiler_cleanup(SETL_SYSTEM_PROTO_VOID) {
    GLOBAL_HEAD=NULL; 
    close_io(SETL_SYSTEM_VOID);
    close_lib(SETL_SYSTEM_VOID);
    I1_FILE=NULL;
    I2_FILE=NULL;
}

#endif
