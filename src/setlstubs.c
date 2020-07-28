
#ifdef MACINTOSH
#undef fprintf	
#undef fputs	
#undef printf	
#endif

#include "system.h"                    /* SETL2 system constants            */


#include "interp.h"                    /* SETL2 interpreter constants       */
#include "compiler.h"
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

#include <setjmp.h>
#include <stdarg.h>

static char print_buffer[100024];


/* Global SETL2 DLL stubs */
/* They MUST be initialized by the DLL user */

FILE *setl_stdin, *setl_stdout, *setl_stderr;

void    (*setl_PostUrl)(char* msg,char *postdata);
void    (*setl_GetUrl)(char* msg);
void    (*setl_JavaScript)(char* msg);

int     (*setl_printf)(const char *format, ...);
int     (*setl_fprintf)(FILE *file, const char *format, ...);
int     (*setl_fputs)(const char *string, FILE *file);
void    (*setl_exit)(int err_code);
void *  (*setl_environment)(char *message);

void	*(*setl_malloc)(size_t size);
void	(*setl_free)(void *ptr);
void    (*setl_event_hook)(void);

char   *setl_lib_file;
char   *javascript_buffer;
int     javascript_buffer_len;

extern jmp_buf abend_env;
extern int hard_stop;
extern int abend_initialized;
char * setl_err_string(int);


int setl_set_io(FILE *_stdin, FILE *_stdout, FILE *_stderr)
{
	setl_stdin = _stdin;
	setl_stdout = _stdout;
	setl_stderr = _stderr;
	
	return 0;
}

/* Functions semi-stubs */

void PostUrl(char* msg, char *postdata) 
{
	setl_PostUrl(msg, postdata);
}

void GetUrl(char* msg)
{
	setl_GetUrl(msg);
}


void JavaScript(char* msg)
{
	setl_JavaScript(msg);
}

void plugin_exit(SETL_SYSTEM_PROTO int err_code)
{
	hard_stop=1;
	runtime_cleanup(SETL_SYSTEM_VOID);
	setl_exit(err_code);
}

void plugin_event_hook(void)
{
	if (setl_event_hook) setl_event_hook();
}

int plugin_fputs(const char *string, FILE *file)
{
	if (file==stdout) {
		return setl_fputs(string,setl_stdout);
	} else if (file==stderr) {
		return setl_fputs(string,setl_stderr);
	} else 	return fputs(string,file);
	
}

int plugin_fprintf(FILE *file, const char *format, ...)
{
	va_list args;
	int n;
		
	va_start(args, format);
	n = vsprintf(print_buffer,format,args);
	va_end(args);
	
	if (file==stdout) {
		setl_fprintf(setl_stdout,print_buffer);
	} else if (file==stderr) {
		setl_fprintf(setl_stderr,print_buffer);
	} else 	fprintf(file,print_buffer);
	
	return n;
}

int plugin_printf(const char *format, ...)
{
	va_list args;
	int n;

   
	va_start(args, format);
	n = vsprintf(print_buffer,format,args);
	va_end(args);
	
	setl_printf(print_buffer);
	
	return n;
}

extern int hard_stop;

void setl_set_fname(SETL_SYSTEM_PROTO char *file_name)
{
	strcpy(C_SOURCE_NAME, file_name);
}



#ifdef PLUGIN
int set_lib_file(SETL_SYSTEM_PROTO char *f) {
char *l;
   if ((f!=NULL) && (strlen(f)>0) &&
       ((l=malloc(strlen(f)))!=NULL)) {
      strcpy(l,f);
      DEFAULT_LIBRARY=l;
      return NO;
     }
   return YES;
}

int set_lib_path(SETL_SYSTEM_PROTO char *f) {
char *l;
   if ((f!=NULL) && (strlen(f)>0) &&
       ((l=malloc(strlen(f)))!=NULL)) {
      strcpy(l,f);
      LIBRARY_PATH=l;
      return NO;
     }
   return YES;
}

#endif

void giveup(SETL_SYSTEM_PROTO char *format, ...)
{
	va_list args;
	      int n;

   
	va_start(args, format);
	n = vsprintf(print_buffer,format,args);
	va_end(args);
	
	setl_fprintf(setl_stderr,print_buffer);
	setl_fprintf(setl_stderr,"\n");

	hard_stop=1;
        if (abend_initialized==1) {
            abend_initialized=0;
	    longjmp(abend_env,1);
	} else setl_exit(1);

}


#if !HAVE_BZERO && HAVE_MEMSET
void  bzero(void *buf,int bytes) {
   memset(buf,0,bytes);
}
#endif

char *get_abend_message(SETL_SYSTEM_PROTO_VOID)
{
	return ABEND_MESSAGE;
}


int get_num_errors(SETL_SYSTEM_PROTO_VOID)
{
   return setl_num_errors();
  
}
int setl_total_error_count(SETL_SYSTEM_PROTO_VOID)
{
   return TOTAL_ERROR_COUNT;
  
}

int setl_total_warning_count(SETL_SYSTEM_PROTO_VOID)
{
   return TOTAL_WARNING_COUNT;
  
}

char *get_err_string(SETL_SYSTEM_PROTO int i)
{
    return setl_err_string(i);
}

int set_compiler_options(SETL_SYSTEM_PROTO char *option,void *flag)
{
  if (strcmp(option,"verbose")==0) {
    VERBOSE_MODE=(int)(flag);
    return 0;
  }
   
  if (strcmp(option,"debugfile")==0) {
    DEBUG_FILE=(FILE *)flag;
    return 0;
  }
   
  if (strcmp(option,"dump")==0) {
    EX_DEBUG=(int)(flag);
    return 0;
  }

  if (strcmp(option,"check_dump")==0) {
    if (EX_DEBUG)
      return 1;      
    return 0;
  }

  if (strcmp(option,"alloc")==0) {
    ALLOC_DEBUG=(int)(flag);
    return 0;
  }
   
  if (strcmp(option,"check_alloc")==0) {
    if (ALLOC_DEBUG)
      return 1;      
    return 0;
  }

  if (strcmp(option,"profiler")==0) {
    PROF_DEBUG=(int)(flag);
    return 0;
  }

  if (strcmp(option,"check_profiler")==0) {
    if (PROF_DEBUG)
      return 1;          
    return 0;
  }

   if (strcmp(option,"assert")==0) {
   		ASSERT_MODE=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"web")==0) {
   		SAFE_MODE=1;
		SAFE_PREFIX=strdup((char*)(flag));
   		return 0;
   }
   if (strcmp(option,"markup")==0) {
   		MARKUP_SOURCE=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"listing")==0) {
   		GENERATE_LISTING=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"lex")==0) {
   		LEX_DEBUG=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"proctab")==0) {
   		PROCTAB_DEBUG=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"tab_width")==0) {
   		TAB_WIDTH=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"process_slice")==0) {
   		PROCESS_SLICE=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"parser")==0) {
   		PRS_DEBUG=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"symtab")==0) {
   		SYM_DEBUG=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"quads")==0) {
   		QUADS_DEBUG=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"ast")==0) {
   		AST_DEBUG=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"code")==0) {
   		CODE_DEBUG=(int)(flag);
   		return 0;
   }
   if (strcmp(option,"optimizer_single")==0) {
   	OPTIMIZE_OF = (int)(flag);
   		return 0;
   }
   if (strcmp(option,"optimizer")==0) {
   		OPTIMIZE_ASSOP = (int)(flag);
        OPTIMIZE_OF = (int)(flag);
   		return 0;
   }
   if (strcmp(option,"step_debug")==0) {   		
        STEP_DEBUG = (int)(flag);
   		return 0;
   }
   if (strcmp(option,"implicit")==0) {
   		IMPLICIT_DECLS = (int)(flag);
   		return 0;
   }
   if (strcmp(option,"intermediate")==0) {
   	USE_INTERMEDIATE_FILES = (int)(flag);
   		return 0;
   }

   if (strcmp(option,"check")==0) {
   	SAFETY_CHECK = (int)(flag);
   		return 0;
   }

   if (strcmp(option,"trace_copies")==0) {
   	TRACE_COPIES= (int)(flag);
   		return 0;
   }

   if (strcmp(option,"set_compiler")==0) {
     if(!(int)(flag))
       COMPILER_OPTIONS = COMPILER_OPTIONS | VERBOSE_FILES;
     else
       COMPILER_OPTIONS = COMPILER_OPTIONS | VERBOSE_OPTIMIZER;
   		return 0;
   }
 
     

   return 1;
}
 
