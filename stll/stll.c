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
 *  Since the library utility is a relatively simple program, the main
 *  program does most of the work outside the library manager files.  We
 *  just parse the command line, processing any commands we find.
\*/

#if MPWC
#pragma segment main
#endif


/* standard C header files */

#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>                    /* variable argument macros          */
#include <ctype.h>                     /* character macros                  */
#include <signal.h>                    /* signal macros                     */
#include <getopt.h>

/* SETL2 system header files */

#include <setlshell.h>


/* constants */

#define YES          1                 /* true constant                     */
#define NO           0                 /* false constant                    */


#define STLLENV_OPTIONS "STLL_OPTIONS" /* environment option string         */

/* forward declarations */
static void stll_exit(int err_code);
static int get_yes_no(char *, ...);    /* query user                        */

#ifdef TSAFE
void *setl_instance;
#endif

void user_interrupt(
   int interrupt_num)

{

   giveup(SETL_SYSTEM "\n*** Interrupted ***");

}

/*\
 *  \function{main()}
 *
 *  This function reads the commands in the form of command line options,
 *  and processes them.  The commands are all fairly simple, given the
 *  facilities of the library manager.
\*/

static  int help = 0;

int main(int argc,                    /* number of command line arguments  */
         char **argv)                  /* command line argument vector      */

{

 int c;                                /* opt_var used by getopt_long()   */ 
   void user_interrupt(int);              /*^C trap     */
   setl_printf  =  printf;
   setl_fprintf = fprintf;
   setl_fputs = fputs;
   setl_environment = NULL;
   setl_exit = stll_exit;
   
#ifdef TSAFE
   setl_instance=Setl_Initialize();
#else
   Setl_Initialize();
#endif

   
   /* display copyright notice */

	Setl_PrintVersion(SETL_SYSTEM_VOID);

   /* set ^C trap */

   
     if (signal(SIGINT,user_interrupt) == SIG_ERR)
      giveup(SETL_SYSTEM "Could not set user interrupt trap!");

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
      c = getopt_long (argc, argv,"c:",long_options, &option_index);
      
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

	  case 'c':
	    /* create a new library file */
	      
	    if (os_access(optarg,0) == 0 &&
		!get_yes_no("%s exists.  Overwrite? ",optarg))
		continue;	      
	      create_lib_file(SETL_SYSTEM optarg);
	    break;
	     
	  case '?':
	    /* getopt_long already printed an error message. */
	    printf("Try 'stll --help' for more information\n");
	    exit(1);
	    break;
	 
	  default:
	    abort ();
	  }	   	    
    }
     
     if(help)
       {
	 printf("Usage: stll [OPTIONS]... LIBRARY_NAME\n"
"stll creates a new SETL2 library LIBRARY_NAME.\n \n"
"   -c           %s\n  --help        %s\n", 
"create a new library file","show this help and exit");
       } 
     
}


/*\
 *  \function{get\_yes\_no()}
 *
 *  This function displays a message for the operator, and demands a yes
 *  or no response.  It will return 1 or 0 according to the operator's
 *  answer.
\*/

static int get_yes_no(
   char *message,                      /* first argument of message         */
   ...)                                /* any other arguments               */

{
va_list arg_pointer;                   /* argument list pointer             */
char prompt[100];                      /* prompt string                     */
char answer[5];                        /* typed answer from user            */
char *p;                               /* temporary looping variable        */


   va_start(arg_pointer,message);
   vsprintf(prompt,message,arg_pointer);
   va_end(arg_pointer);


   /* wait for yes or no */

   for (;;) {

      /* display the prompt, and get a response */

      fflush(stdin);
      fputs(prompt,stderr);
      fflush(stderr);
      fgets(answer,5,stdin);
      fflush(stdin);

      /* strip the carriage return and convert to lower case */

      answer[4] = '\0';
      for (p = answer; *p; p++) {
         if (*p == '\n')
            *p = '\0';
         if (isupper(*p))
            *p = tolower(*p);
      }

      /* return if the answer is yes or no */

      if (strcmp(answer,"y") == 0 || strcmp(answer,"yes") == 0)
         return YES;

      if (strcmp(answer,"n") == 0 || strcmp(answer,"no") == 0)
         return NO;

      /* display an error message and continue */

      fputs("\nPlease answer yes or no.\n",stderr);

   }
}

void stll_exit(int err_code)
{
  exit(err_code);
}
