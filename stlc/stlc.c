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

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>                 /* file types                        */
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>                  /* file status                       */
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>                     /* file functions                    */
#endif


#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>                     /* character macros                  */
#include <signal.h>                    /* signal macros                     */
#include <getopt.h>

/* SETL2 system header files */

#include <setlshell.h>


/* constants */

static int file_count;                 /* number of source files processed  */


/* forward declarations */

static void stlc_exit(int err_code);

static void compile_file(char *);      /* compile a single file             */

#ifdef TSAFE
void * setl_instance;
#endif

static  int help = 0,version = 0;

void user_interrupt(
   int interrupt_num)

{

   giveup(SETL_SYSTEM "\n*** Interrupted ***");

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

int main(
   int argc,                           /* number of command line arguments  */
   char *argv[])                       /* command line argument vector      */
{
char *temp_path = "";                  /* path for temporary files          */
char file_path[PATH_LENGTH + 1];       /* path name of source files         */
filelist_ptr_type fl_head;             /* list of matching files            */
filelist_ptr_type fl_ptr;              /* used to loop over file list       */
char *unit_file_name;                  /* file name for file unit           */
char *p;                               /* temporary looping variable        */
 int c;                                /* opt_var used by getopt_long()     */
 char *u_optarg = NULL;                /* further argument passed to -u opt */

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
 setl_exit = stlc_exit;
 setl_lib_file = "setl2.lib";

 compiler_init(SETL_SYSTEM_VOID);

   /* display copyright notice */

   Setl_PrintVersion(SETL_SYSTEM_VOID);

   /* set ^C trap */

   if (signal(SIGINT,user_interrupt) == SIG_ERR)
      giveup(SETL_SYSTEM "Could not set user interrupt trap!");

#ifdef HAVE_SIGNAL
#ifdef DEBUG

   if (signal(SIGSEGV,c_segment_error) == SIG_ERR)
      giveup(SETL_SYSTEM msg_trap_segment);

#endif
#endif

   /* Set verbose mode */
   set_compiler_options(SETL_SYSTEM "verbose",2 );
   
   /* loop through arguments */
   while (1)
    {
      static struct option long_options[] =
        {
          /* These options don't set a flag.
             We distinguish them by their indices. */
          {"help", 0, &help, 1},
          {"version", 0, &version, 1},
          {0, 0, 0, 0}
        };

      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv,"gifsmnl:p:t:u:o:v:d:",long_options, &option_index);
      
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

	  case 'g':
	    /* toggle symtab mode */
	    
	    set_compiler_options(SETL_SYSTEM "symtab", 1);
	    break;

	  case 'i':
	    /* toggle implicit variable declarations switch */
	    
	    set_compiler_options(SETL_SYSTEM "implicit",1);
	    break;

	  case 'f':
	    /* toggle intermediate file switch */

	    set_compiler_options(SETL_SYSTEM "intermediate",1);
	    break;

	  case 's':
	    /* toggle listing switch */

	    set_compiler_options(SETL_SYSTEM "listing",1);
	    break;

	  case 'm':
	    /* toggle source markup switch */

	    set_compiler_options(SETL_SYSTEM "markup",1);
	    break;

	  case 'n':
	    /* toggle safety check switch */

	    set_compiler_options(SETL_SYSTEM "check",1);
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

	  case 't':
	    /* change tab width */

   	    set_compiler_options(SETL_SYSTEM "tab_width",atoi(optarg) );
	    break;

	  case 'u':
	    /* add file unit */
	    	    	   
	    unit_file_name = malloc(strlen(optarg) + 1);
	    if (unit_file_name == NULL)
	      giveup(SETL_SYSTEM msg_malloc_error);
	    strcpy(unit_file_name,optarg);

	    if(optind < argc)
		u_optarg = argv[optind];

	    if (u_optarg == NULL || u_optarg[0] == '-')
	      giveup(SETL_SYSTEM "Missing unit name in file unit");
         
	    add_file_unit(SETL_SYSTEM unit_file_name, u_optarg);
	    free(unit_file_name);
	    break;

	  case 'o':
	    /* set optimizer flags */
	    
	    /* toggle each switch */
	    for (p = optarg; *p; p++) 
	      {
		switch (*p) 
		  {
		  case '1' :
                    set_compiler_options(SETL_SYSTEM "optimizer_single", 1);
		    break;

		  case '2' :
                    set_compiler_options(SETL_SYSTEM "optimizer", 1);
		    break;

		  default :
		    giveup(SETL_SYSTEM "Invalid optimizer option => %c",*p);
		  }
	      }
	    
	    break;

	  case 'v':
	    /* set verbose flags */
	    
	    /* toggle each switch */
	    for (p = optarg; *p; p++) 
	      {

		switch (*p) 
		  {

		  case 'f' :
		    set_compiler_options(SETL_SYSTEM "set_compiler", 0);
		    break;

		  case 'o' :
		    set_compiler_options(SETL_SYSTEM "set_compiler", 1);
		    break;

		  default :
		    giveup(SETL_SYSTEM "Invalid -v option => %c",*p);
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
               
		      case 'x' :
			set_compiler_options(SETL_SYSTEM "lex",1); 
			break;

		      case 'l' :
			set_compiler_options(SETL_SYSTEM "listing",1); 
			break;

		      case 'k' :
			set_compiler_options(SETL_SYSTEM "proctab", 1);
			break;

		      case 'p' :
			set_compiler_options(SETL_SYSTEM "parser", 1);
			break;

		      case 's' :
			set_compiler_options(SETL_SYSTEM "symtab",1);
			break;

		      case 'q' :
			set_compiler_options(SETL_SYSTEM "quads",1);
			break;

		      case 'a' :
			set_compiler_options(SETL_SYSTEM "ast",1);          
			break;

		      case 'c' :
			set_compiler_options(SETL_SYSTEM "code",1);
			break;

		      default :
			giveup(SETL_SYSTEM "Invalid debugging option => %c",*p);
		      }
		  }
	  
#endif
	    break;

	  case '?':
	    /* getopt_long already printed an error message. */
	    printf("Try 'stlc --help' for more information\n");
	    break;
	 
	  default:
	    abort ();
	  }
    }

   if(version)
     {
       exit(1);
     }

   if(help)
       {
	 printf("Usage: stlc [OPTIONS]... FILE_NAMES\n"
"stlc compiles the specified files into a SETL2 library\n"
"   -g                        %s\n   -i                        %s\n"
"   -f                        %s\n   -s                        %s\n"
"   -m                        %s\n   -n                        %s\n"
"   -l                        %s\n   -p                        %s\n"
"   -u  FILE_NAME UNIT_FILE   %s\n"
"   -o  1 or 2                %s\n"
"   -v  f                     %s\n"  
"       o                     %s\n"
"   -d  x                     %s\n"  
"       l                     %s\n"  
"       k                     %s\n"  
"       p                     %s\n"  
"       s                     %s\n"  
"       q                     %s\n"  
"       a                     %s\n"  
"       c                     %s\n"  
" --version                   %s\n"
" --help                      %s\n",
"toggle symtab mode","toggle implicit variable declarations switch","toggle intermediate file switch","toggle listing switch","toggle source markup switch","toggle safety check switch","change default library","change library path","add file unit","set optimizer flags","set verbose flags: files","set verbose flags: optimizer","set debugging flags: lex","set debugging flags: listing","set debugging flags:proctab","set debugging flags: parser","set debugging flags: symtab","set debugging flags: quads","set debugging flags: ast","set debugging flags:code","show this version and then exit","show this information and then exit");
       } 

   /*
    *  At this point, we must have a file specification.  Compile any
    *  matching files.
    */

    file_count=0;
    if (optind < argc)
	{
	  while (optind < argc)
	    {
	      strcpy(file_path,argv[optind++]);	      

#if VMS

	      for (p = file_path + strlen(file_path) - 1;
		   p >= file_path && *p != '.' && *p != "]";
		   p--);
#else

	      for (p = file_path + strlen(file_path) - 1;
		   p >= file_path && *p != '.' && *p != PATH_SEP;
		   p--);

#endif

	      if (p < file_path || *p != '.')
		strcat(file_path,".stl");

	      fl_head = setl_get_filelist(SETL_SYSTEM file_path);
	      if (fl_head == NULL)
		giveup(SETL_SYSTEM "No files match %s",file_path);

	      for (fl_ptr = fl_head; 
			   fl_ptr != NULL; 
			   fl_ptr = setl_get_next_file(fl_ptr) ) {
					compile_file(setl_get_filename(fl_ptr));
					file_count++;
			  }

	      setl_free_filelist(fl_head);
	    }
            if (file_count > 1) {

               printf("Summary:\n   %3d errors\n   %3d warnings\n\n",
                      setl_total_error_count(SETL_SYSTEM_VOID),
					  setl_total_warning_count(SETL_SYSTEM_VOID));

            }
	} 
   
}


static void compile_file(
   char *source_file)                  /* source file name                  */

{

  FILE *file;
  char *buffer;
  long size_file;
  


  int status;

  if(!(file = fopen(source_file,"rb")))
     giveup(SETL_SYSTEM "WARNING: %s not found!",source_file);


  fseek(file,0,SEEK_END);
  size_file = ftell(file);
  fseek(file,0,SEEK_SET);
  
  if(!(buffer = (char *) malloc(sizeof(char)*size_file+1)))
     giveup(SETL_SYSTEM msg_malloc_error);
  
  fread(buffer,size_file,1,file);
  buffer[size_file]=0;

  runtime_cleanup(SETL_SYSTEM_VOID);
  
  setl_set_fname(SETL_SYSTEM source_file);
  status=compile_fragment(SETL_SYSTEM buffer,0); 

  free(buffer);
  fclose(file);
}


void stlc_exit(int err_code)
{
  exit(err_code);
}

