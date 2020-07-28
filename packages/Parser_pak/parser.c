/*\
 *
 *
 *  MIT License
 * 
 *  Copyright (c) 2001 Salvatore Paxia
 * 
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 *  This is a Parser Native Package for SETL2
 *
 */


/* SETL2 system header files */


/* Native Packages Macros */

#include "macros.h"

/* Compiler include files */

#include "compiler.h"
#include "ast.h"
#include "proctab.h"
#include "listing.h"
#include "namtab.h"
#include "symtab.h"



/* constants */

#define YES         1                  /* true constant                     */
#define NO          0                  /* false constant                    */

ast_ptr_type ast_root;                 /* root of tree being processed      */
int parse_progorexpr;
int skip_errors;
int ignore_errors;
int program_nolines;

int check_int(
  SETL_SYSTEM_PROTO
  specifier *argv,                  
  int param,
  int type,
  char *typestr,
  char *routine)
{

 if (argv[param].sp_form == ft_short) 
      return (int)(argv[param].sp_val.sp_short_value);
   else if (argv[param].sp_form == ft_long) 
      return (int)long_to_short(SETL_SYSTEM argv[param].sp_val.sp_long_ptr);
   else 
      abend(SETL_SYSTEM msg_bad_arg,"integer",param+1,routine,
            abend_opnd_str(SETL_SYSTEM argv+param));
       
}



int compile(
   SETL_SYSTEM_PROTO
   char *program_source)
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
   TOTAL_ERROR_COUNT = 0;
   TOTAL_WARNING_COUNT = 0;
   COMPILING_EVAL = YES;
   SAFETY_CHECK = 0;


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


   if (I2_FILE==NULL) {
       strcpy(I2_FNAME, MEM_LIB_NAME); 
       create_lib_file(SETL_SYSTEM I2_FNAME);
    }
    strcpy(C_SOURCE_NAME,"<parser>");

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
      if (I2_FILE==NULL) { 
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

   /* generate code, write to work library */
   
   ast_root = NULL;

   for (proctab_ptr = predef_proctab_ptr->pr_child;
        proctab_ptr != NULL;
        proctab_ptr = proctab_ptr->pr_next) {

      /* reset error counts */

      UNIT_ERROR_COUNT = 0;
      UNIT_WARNING_COUNT = 0;
      
      if ((proctab_ptr->pr_type == pr_program)&&
	  (FILE_ERROR_COUNT ==0))
         ast_root = load_ast(SETL_SYSTEM &(proctab_ptr->pr_body_code));



      /* generate quadruples
         gen_quads(SETL_SYSTEM proctab_ptr);
 */

   }
   /* print summary statistics */

   if (COMPILING_EVAL==YES) {
      if (FILE_ERROR_COUNT == 0) {
         return (SUCCESS_EXIT);
     }
     else {
        TOTAL_GLOBAL_SYMBOLS = save_global_symbols;
        GLOBAL_HEAD = save_global_head;
        return (COMPILE_ERROR_EXIT);
     }
   }

}



string_h_ptr_type setl2_string(SETL_SYSTEM_PROTO char *s,int slen)
{
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
char *string_char_ptr, *string_char_end;

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = NULL;
   string_char_ptr = string_char_end = NULL;

   /* copy the source string */

   while (slen-->0) {

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
   return string_hdr;
}

struct tuple_h_item *return_subtree(
   SETL_SYSTEM_PROTO
   ast_ptr_type ast_root,              /* ast_root of tree to be printed    */
   int ast_label,
   namtab_ptr_type nt
   )
{
TUPLE_CONSTRUCTOR(ca)
ast_ptr_type ast_ptr;                  /* used to loop over nodes           */
symtab_ptr_type symtab_ptr;            /* work symbol table pointer         */
char print_symbol[16];                 /* truncated symbol for printing     */
int i;                                 /* temporary looping variable        */
int typ;
struct tuple_h_item *sub;
specifier s;

  
   if (ast_root==NULL) {
   
       //(ast_label==ast_placeholder) ){
   
       TUPLE_CONSTRUCTOR_BEGIN(ca);

            s.sp_form = ft_string;
            s.sp_val.sp_string_ptr = 
							setl2_string(SETL_SYSTEM ast_desc[ast_label],
			    			 strlen(ast_desc[ast_label]));
			    	TUPLE_ADD_CELL(ca,&s);
           
  			TUPLE_CONSTRUCTOR_END(ca);

  	  return TUPLE_HEADER(ca);

   
   }
   
//   if (ast_root == NULL) {
//      return NULL;
//   }
   /* we loop over nodes at the current level */

  
   if ((ast_label<0)&&((ast_root->ast_type)==ast_list)) {
       typ=ast_root->ast_type;
       
       
       if (((typ==ast_uminus)||(typ==ast_sub))&&(ast_root->ast_extension!=NULL))
       	
       	 typ=1025;
       	 
       return return_subtree(SETL_SYSTEM ast_root->ast_child.ast_child_ast,typ,ast_root->ast_extension);

   }

   TUPLE_CONSTRUCTOR_BEGIN(ca);

   
   if (ast_label>0) {
   		if (ast_label>1024) {
   				  s.sp_form = ft_string;
            s.sp_val.sp_string_ptr = setl2_string(SETL_SYSTEM 
            		nt->nt_name,strlen(nt->nt_name));
            TUPLE_ADD_CELL(ca,&s);
            
   		} else {
            s.sp_form = ft_string;
            s.sp_val.sp_string_ptr = 
								setl2_string(SETL_SYSTEM ast_desc[ast_label],strlen(ast_desc[ast_label]));
						TUPLE_ADD_CELL(ca,&s);
      }
   }

   for (ast_ptr = ast_root; ast_ptr != NULL; ast_ptr = ast_ptr->ast_next) {

      switch (ast_ptr->ast_type) {

         case ast_namtab :

            s.sp_form = ft_string;
            s.sp_val.sp_string_ptr = 
							setl2_string(SETL_SYSTEM (ast_ptr->ast_child.ast_namtab_ptr)->nt_name,
                     strlen((ast_ptr->ast_child.ast_namtab_ptr)->nt_name));
            TUPLE_ADD_CELL(ca,&s);
            break;

         case ast_symtab :

            symtab_ptr = ast_ptr->ast_child.ast_symtab_ptr;

            /* build up a junk symbol for temporaries and labels */

            if (symtab_ptr->st_namtab_ptr == NULL) {

               if (symtab_ptr->st_type == sym_label)
                  sprintf(print_symbol,"$L%ld",(long)symtab_ptr);
               else
                  sprintf(print_symbol,"$T%ld",(long)symtab_ptr);

			            s.sp_form = ft_string;
			            s.sp_val.sp_string_ptr = 
									setl2_string(SETL_SYSTEM 
									     print_symbol,
							                     strlen(print_symbol));
			            TUPLE_ADD_CELL(ca,&s);
            
            }

            else {

		            s.sp_form = ft_string;
		            s.sp_val.sp_string_ptr = 
								setl2_string(SETL_SYSTEM 
		                     (symtab_ptr->st_namtab_ptr)->nt_name,
		                     strlen((symtab_ptr->st_namtab_ptr)->nt_name));
		            TUPLE_ADD_CELL(ca,&s);

            }


            break;

         default :
						typ=ast_ptr->ast_type;    
     			  if (((typ==ast_uminus)||(typ==ast_sub))&&(ast_ptr->ast_extension!=NULL))
      			 	 typ=1025;
						
            sub=return_subtree(SETL_SYSTEM ast_ptr->ast_child.ast_child_ast,typ,ast_ptr->ast_extension);
				    if (sub) {
			                 s.sp_form = ft_tuple;
			                 s.sp_val.sp_tuple_ptr = sub;
			                 TUPLE_ADD_CELL(ca,&s);
			                 
				    }
            break;

      }
   }
  TUPLE_CONSTRUCTOR_END(ca);

  return TUPLE_HEADER(ca);

}


/***********************************************************************/

SETL_API void PARSE(
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
unittab_ptr_type unit_ptr;  
struct instruction_item *pc_old;      
char *fragment;                        /* fragment to compile               */
int compile_result;
char eval_name[32];
global_ptr_type temp_global_head;
struct tuple_h_item *ss;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"parse",
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

   
   compile_result=compile(SETL_SYSTEM key);  
   parse_progorexpr=0; /* PROGRAM */
   free(key);
   skip_errors=0;
   ignore_errors=0;

  
   ss = NULL;
   if (ast_root) 
	    ss=return_subtree(SETL_SYSTEM ast_root,-1,NULL);

   unmark_specifier(target);
   if (ss) {
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = ss;
   } else {
      target->sp_form = ft_omega;

   }

   return;
}


SETL_API void PARSE_EXPR(
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
unittab_ptr_type unit_ptr;  
struct instruction_item *pc_old;      
char *fragment;                        /* fragment to compile               */
int compile_result;
char eval_name[32];
global_ptr_type temp_global_head;
struct tuple_h_item *ss;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"parse_expr",
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

   sprintf(fragment,"program dummy;\n%s\nend dummy;",key);
   free(key);
   program_nolines=1;
   t=fragment;
   while (*t!=0) {
   char ignore_cr;
        if ((*t=='\r')||(*t=='\n')) {
          if (*t=='\r') ignore_cr='\n'; else ignore_cr='\r';
          t++;
                while ((*t!=0)&&(*t==ignore_cr)) t++;
                program_nolines++;
        }
        t++;
   }

   compile_result=compile(SETL_SYSTEM fragment);
   free(fragment);
   parse_progorexpr=1; /* EXPRESSION */
   ss = NULL;


   if (ast_root) 
	    ss=return_subtree(SETL_SYSTEM ast_root,-1,NULL);

   unmark_specifier(target);
   if (ss) {
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = ss;
   } else {
      target->sp_form = ft_omega;

   }

   return;
}

SETL_API void COMPILE(
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
unittab_ptr_type unit_ptr;  
struct instruction_item *pc_old;      
char *fragment;                        /* fragment to compile               */
int compile_result;
char eval_name[32];
global_ptr_type temp_global_head;
struct tuple_h_item *ss;

   /* convert the key to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"compile",
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

   
   
  
   compile_result=compile_fragment(SETL_SYSTEM key,2);  /* Compiling a fragment */ 
   free(key);
   if (compile_result!=SUCCESS_EXIT) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
   }
   
   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value = 0;
   return;
}

SETL_API void SETL_NUM_ERRORS(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{

  unmark_specifier(target);
  target->sp_form = ft_short;
  target->sp_val.sp_short_value = setl_num_errors();
  return;

}

char *setl_err_string(int e);

SETL_API void SETL_ERR_STRING(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
STRING_CONSTRUCTOR(sa)
int e;
char *s,*q;
int z;

  e=check_int(SETL_SYSTEM argv,0,ft_short,"integer","setl_err_string");
  q=s=setl_err_string(e);
 
  z=strlen(s);
  STRING_CONSTRUCTOR_BEGIN(sa);
  while (z>0) {			
			   STRING_CONSTRUCTOR_ADD(sa,(*q));
	 				 q++;z--;
  } 
  
  unmark_specifier(target);
  target->sp_form = ft_string;
  target->sp_val.sp_string_ptr = STRING_HEADER(sa);
  return;

}




