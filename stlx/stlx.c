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
 *
 *  \package{Main}
 *
 *  This file contains the main function of the SETL2 interpreter.  We
 *  initialize all the tables and process command line options.  All
 *  argments before the program name are assumed to be for the
 *  interpreter itself.  Others are gathered into a tuple for use by the
 *  SETL2 program.
\*/



/* standard C header files */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>                     /* character macros                  */
#include <signal.h>                    /* signal macros                     */


#include <setlshell.h>


/* constants */

#define MAX_PROGRAM_NAME_LEN    64

/* forward & external declarations */

static void stlx_exit(int err_code);



static  int help = 0;
static  FILE *debug_file;

#ifdef TSAFE
void *setl_instance;
#endif


void user_interrupt(
   int interrupt_num)

{

   giveup(SETL_SYSTEM "\n*** Interrupted ***");

}

int create_debug_file(char *f) {
  debug_file=fopen(f,"w");
  if (debug_file==NULL)       
      return NO;
  return YES;
}


/*\
 *  \function{main()}
 *
 *  This function executes the interpreter.  First we process command
 *  line options, we build a tuple out of options following the file
 *  name, we load the program, and we execute it.
\*/

int main(
   int argc,                           /* number of command line arguments  */
   char **argv)                        /* command line argument vector      */
{
char program[MAX_PROGRAM_NAME_LEN + 1];       /* program to be executed            */
char *p;
int c;                                /* opt_var used by getopt_long()     */

#ifdef TSAFE
   setl_instance=Setl_Initialize();
#else
   Setl_Initialize();
#endif

 setl_set_io(stdin,stdout,stderr);
 setl_printf = printf;
 setl_fprintf = fprintf;
 setl_fputs = fputs;
 setl_environment = NULL;
 setl_lib_file = "setl2.lib";
 setl_exit = stlx_exit;

 compiler_init(SETL_SYSTEM_VOID);

   /* set ^C trap */

   if (signal(SIGINT,user_interrupt) == SIG_ERR)
      giveup(SETL_SYSTEM "Could not set user interrupt trap!");
  
#ifdef HAVE_SIGNAL
#ifdef DEBUG

   if (signal(SIGSEGV,i_segment_error) == SIG_ERR)
      giveup(SETL_SYSTEM msg_trap_segment);

#endif
#endif


   /* Set verbose mode & other options */
   
   Setl_SetVerboseMode(SETL_SYSTEM YES); 

   /* loop through arguments */

   while (1) 
     {
       static struct option long_options[] =
        {
          /* These options don't set a flag.
             We distinguish them by their indices. */
          {"help", 0, &help, 1},
          {"version", 0, 0, 0},
          {0, 0, 0, 0}
        };

      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv,"vl:p:ms:a:d:",long_options, &option_index);
      
      /* Detect the end of the options. */
      if (c == -1)
        break;

	switch (c)
	  {
	  case 0:
	    /* If this option set a flag, do nothing else now. */
	    if (long_options[option_index].flag != 0)
	      break;
	    printf ("option %s", long_options[option_index].name);
	    if (optarg)
	      printf (" with arg %s", optarg);
	    printf ("\n");
	    break;

	  case 'v':
	    /* print out the version number */
	    
	    Setl_PrintVersion(SETL_SYSTEM_VOID);
	    exit(1);
	    break;

	  case 'l':
	    /* change default library */

	    if (set_lib_file(SETL_SYSTEM optarg))
	      giveup(SETL_SYSTEM msg_malloc_error);
	    
	    break;
      
	  case 'p':
	    /* change library path */
	    
	    if (set_lib_path(SETL_SYSTEM optarg))	    
	      giveup(SETL_SYSTEM msg_malloc_error);
	   
	    break;
         
	  case 'm':
	    /* toggle source markup switch */

	    set_compiler_options(SETL_SYSTEM "markup",(void*)1);
	    break;

	  case 's':
	    /* set slice size */
	    
	    set_compiler_options(SETL_SYSTEM "process_slice",(void*)atol(optarg));
	    break;

	  case 'a':
	    /* set assert flags */
	    	    
	    /* toggle each switch */

	    for (p = optarg; *p; p++) 
	      {

		switch (*p) 
		  {
		  case 'f' :
		    set_compiler_options(SETL_SYSTEM "assert", (void*)1);	    
		    break;

		  case 'l' :
		    set_compiler_options(SETL_SYSTEM "assert", (void*)2);
		    break;

		  default :
		    giveup(SETL_SYSTEM msg_bad_assert_opt,*p);
		  }
	      }
	    break;
        
	  case 'd':
	    /* set debugging flags */

#ifdef DEBUG
	    
	    /* toggle each switch */

	    for (p = optarg; *p; p++) 
	      {

		switch (*p) 
		  {

		  case 'a' :		    
		    set_compiler_options(SETL_SYSTEM "alloc", (void*)1);
		    break;

		  case 'x' :		    
		    set_compiler_options(SETL_SYSTEM "dump", (void*)1);
		    break;

		  case 's' :
		    set_compiler_options(SETL_SYSTEM "step_debug", (void*)1);
		    break;

		  case 'p' :
		    set_compiler_options(SETL_SYSTEM "profiler", (void*)1);
		    break;

		  case 'd' :
		    if(!create_debug_file("setl2.dbg"))
		      giveup(SETL_SYSTEM msg_bad_debug_file);
			set_compiler_options(SETL_SYSTEM "debugfile", (void*)debug_file);
		    break;

		  case 'c' :
		    set_compiler_options(SETL_SYSTEM "trace_copies", (void*)1);
		    break;
 
		  default :
		    giveup(SETL_SYSTEM msg_bad_debug_opt,*p);

		  }
	      }
	    
#endif
	    break;

	  case '?':
	    /* getopt_long already printed an error message. */
	    printf("Try 'stlx --help' for more information\n");
	    exit(1);
	    break;
	 
	  default:
	    abort ();
	  }
     } 
 
   if(help)
       {
	 printf("Usage: stlx [OPTIONS]... PROGRAM_NAME\n"
"stlx executes the specified program.\n"
"   -v         %s\n   -l         %s\n"
"   -p         %s\n   -m         %s\n"
"   -s         %s\n"
"   -a  f      %s\n"
"       l      %s\n"
"   -d  x      %s\n"
"       s      %s\n"
"       p      %s\n"
"       d      %s\n"
"       c      %s\n"
"  --help      %s\n",
		"print out the version number","change default library","change library path","toggle source markup switch","set slice size","set assert flag: fail","set assert flag: log","set debugging flags: dump","set debugging flags: step debug","set debugging flags: profiler","set debugging flags: create a debug file","set debugging flags: trace copies","show this informations and then exit");
	 exit(1);
}
   /*
    *  At this point, we expect to have a program name.
    */

   if (optind < argc)
	{
	   strcpy(program,argv[optind++]);	   
	      /* program names are upper case */

	      for (p = program; *p; p++) 
		{
		  if (islower(*p))
		    *p = toupper(*p);
		}
		  

	      /* initialize tables */
	      runtime_cleanup(SETL_SYSTEM_VOID);
		
	      plugin_main(SETL_SYSTEM program);
		  Setl_SetCommandLine(SETL_SYSTEM argc,optind,argv);
	      execute_go(SETL_SYSTEM 1);

	      runtime_cleanup(SETL_SYSTEM_VOID);
	      profiler_dump(SETL_SYSTEM_VOID);
   
	      exit(0);

	}
   else
     giveup(SETL_SYSTEM msg_missing_prog_name);
}



void stlx_exit(int err_code)
{
  exit(err_code);
}
