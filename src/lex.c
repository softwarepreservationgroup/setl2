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
 *  \package{The Lexical Analyzer}
 *
 *  The lexical analyzer is fairly conventional, so we won't describe it
 *  in much detail.  We just want to point out a few oddities.
 *
 *  We do not follow the regular expression model of lexical analyzers.
 *  In SETL2, as in most other languages, the first character of a token
 *  determines a small set of token classes which must contain the
 *  current token.  The lexical analyzer uses this to derive it's
 *  structure.  Essentially, it is one big case statement, dependent on
 *  the first non-blank character it sees.  It returns directly from the
 *  case item it executes.
 *
 *  The function header of \verb"get_token()" is weird.  In normal use,
 *  we just return from the middle of the function.  During debugging, we
 *  want to print the token we found first.  We implement this option
 *  with conditional compilation.  if the \verb"DEBUG" option is set, we
 *  have a dummy \verb"get_token()" which just calls the real
 *  \verb"get_token()", prints the token it finds, and returns.  If
 *  \verb"DEBUG" is not set, we just return from wherever we find a
 *  complete token.
 *
 *  Second, we make no attempt to find the value of literals here, we
 *  only scan them.  Literals in SETL2 are more complex that in most
 *  languages, since we do not restrict their length other than the
 *  restriction on token lengths.  For this reason, and because we also
 *  need to find these values when loading package specifications, we
 *  call other functions to find literal values.
 *
 *  Finally, we assemble composite symbols for assignment operators and
 *  application operators here, rather than in the grammar.  This is to
 *  facilitate construction of an LALR(1) grammar for SETL2. We have a
 *  problem in the grammar with symbols like \verb"+:=", which leads to
 *  a shift-reduce error if we return \verb"+" and \verb":=".  To
 *  compensate, we save binary operators for one iteration of the main
 *  \verb"while" loop, and and assemble the composites if we find an
 *  assignment symbol or slash after a binary operator.
 *
 *  \texify{lex.h}
 *
 *  \packagebody{Lexical Analyzer}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "chartab.h"                   /* character type table              */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "c_integers.h"                  /* integer literals                  */
#include "c_reals.h"                     /* real literals                     */
#include "c_strngs.h"                    /* string literals                   */
#include "lex.h"                       /* lexical analyzer                  */
#include "listing.h"                   /* source and error listings         */


#ifdef PLUGIN
#define fprintf plugin_fprintf
#endif


/* performance tuning constants */

#define CHAR_BUFF_SIZE        (512+MAX_TOK_LEN)
                                       /* input buffer size                 */

/* forward function declarations */

static void fill_buffer(SETL_SYSTEM_PROTO_VOID);
                                       /* fill the character buffer         */

/* package-global data */

static unsigned char *source_buffer = NULL;
                                       /* source buffer                     */
static unsigned char *start;           /* start position of token           */
static unsigned char *lookahead;       /* lookahead pointer                 */
static unsigned char *endofbuffer;     /* last filled position in buffer    */
static unsigned curr_line;             /* source file position -- line      */
static unsigned char *curr_line_start; /*   -- line starting position       */
static unsigned int curr_col_adjustment;
                                       /*   -- column adjustment            */



static char *extension_map[256]={
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   "BANG", 
   NULL, 
   NULL, 
   "DOLL", 
   "PERCENT", 
   "AMP", 
   "APOS", 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   "AT", 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   "BACKSL", 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   "TILDE", 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   "DAGGER", 
   NULL, 
   "CENT", 
   "BRITPOUND", 
   "PARA", 
   "DOT", 
   "NOTE", 
   "BETA", 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   "INFIN", 
   "PLMIN", 
   NULL, 
   NULL, 
   "YEN", 
   "MU", 
   "DIFF", 
   "SIGMA", 
   "PI", 
   "SMALLPI", 
   "INTEGRAL", 
   NULL, 
   NULL, 
   "OMEGA", 
   NULL, 
   "THORN", 
   NULL, 
   NULL, 
   "NTSGN", 
   NULL, 
   NULL, 
   "APPROXE", 
   "DELT", 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   "DIAMOND", 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   "GRCROSS", 
   "SMALLDOT", 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL 
};



/*\
 *  \function{macros}
 *
 *  We use a few macros in the lexical analyzer, for functions we need to
 *  perform many places but do not want to pay for a procedure call.
\*/

/* advance lookahead macro */

#ifdef TSAFE
#define advance_la \
    {if (++lookahead > endofbuffer) fill_buffer(plugin_instance);}
#else
#define advance_la \
    {if (++lookahead > endofbuffer) fill_buffer();}
#endif

/* token builder macro */

#define build_token(c,s,n) { \
   token->tk_token_class = c; \
   token->tk_token_subclass = s; \
   lexeme_pos.fp_line = curr_line; \
   lexeme_pos.fp_column = start - curr_line_start + 1 + curr_col_adjustment; \
   memcpy(&(token->tk_file_pos),&lexeme_pos,sizeof(file_pos_type)); \
   token->tk_namtab_ptr = n; \
}

/*\
 *  \function{init\_lex()}
 *
 *  This function initializes the lexical analyzer.  It opens the source
 *  file and fills the character buffer.
\*/

void init_lex(SETL_SYSTEM_PROTO_VOID)

{

   if (source_buffer == NULL) {

      source_buffer = (unsigned char *)malloc(
                             (size_t)(CHAR_BUFF_SIZE + MAX_TOK_LEN + 1));
      if (source_buffer == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);

   }

#ifndef DYNAMIC_COMP
   if (SOURCE_FILE != NULL)
      fclose(SOURCE_FILE);

   SOURCE_FILE = fopen(SOURCE_NAME,BINARY_RD);
#endif

   start = lookahead = endofbuffer = source_buffer;

   fill_buffer(SETL_SYSTEM_VOID);

   curr_line = 1;
   curr_line_start = source_buffer;
   curr_col_adjustment = 0;

}

/*\
 *  \function{close\_lex()}
 *
 *  This function closes the lexical analyzer.  It just has to close the
 *  source file and flag it as closed.
\*/

void close_lex(
   SETL_SYSTEM_PROTO_VOID
   )

{

#ifndef  DYNAMIC_COMP
   fclose(SOURCE_FILE);
#endif
   SOURCE_FILE = NULL;

}

/*\
 *  \function{get\_token()}
 *
 *  This is the primary lexical analysis function, get token.  It is an
 *  unusual function in two respects: first, like most equivalent
 *  functions, it is essentially one big case statement.  Since the code
 *  for a case can become quite involved, and we tend to think of these as
 *  independent functions, the indentation style of this case statement is
 *  unconventional.  We indented them as if they were separate functions.
 *
 *  Second, the function header is unusual.  The function returns the
 *  token it finds from wherever it finds it.  In normal use this is
 *  fine.  During debugging, we may want to print the tokens as we read
 *  them however.  We used a dummy function which calls get token, prints
 *  the token, and passes it on.  Since we didn't want the overhead of an
 *  additional call except during debugging, we used conditional
 *  compilation to implement this.
\*/

#ifdef DEBUG

/* debugging code */

static void real_gettok(SETL_SYSTEM_PROTO token_type *); 
                                       /* 'real' get token                  */

void get_token(
   SETL_SYSTEM_PROTO
   token_type *token)

{

   /* call real get token */

   real_gettok(SETL_SYSTEM token);

   /* if a list of tokens is required, print the token */

   if (LEX_DEBUG) {

      if (token->tk_token_class == tok_eof) {

         fprintf(DEBUG_FILE,"LEX : End of file\n");

      }
      else {

         fprintf(DEBUG_FILE,"LEX : %s\n",(token->tk_namtab_ptr)->nt_name);

      }
   }

   return;

}

static void real_gettok(
   SETL_SYSTEM_PROTO
   token_type *token)

#else

/* this is the non-debugging version */

void get_token(
   SETL_SYSTEM_PROTO
   token_type *token)

#endif

{
static char lexeme[MAX_TOK_LEN+10];    /* lexeme buffer                     */
file_pos_type lexeme_pos;              /* file position of lexeme           */
int saving_binop;                      /* YES if we're saving a binary      */
                                       /* operator                          */
static char is_binop[] = {             /* binary operator table             */
/* ## begin is_binop */
   0,                                  /* end of file                       */
   0,                                  /* error token                       */
   0,                                  /* identifier                        */
   0,                                  /* literal                           */
   1,                                  /* keyword => AND                    */
   0,                                  /* keyword => ASSERT                 */
   0,                                  /* keyword => BODY                   */
   0,                                  /* keyword => CASE                   */
   0,                                  /* keyword => CLASS                  */
   0,                                  /* keyword => CONST                  */
   0,                                  /* keyword => CONTINUE               */
   0,                                  /* keyword => ELSE                   */
   0,                                  /* keyword => ELSEIF                 */
   0,                                  /* keyword => END                    */
   0,                                  /* keyword => EXIT                   */
   0,                                  /* keyword => FOR                    */
   0,                                  /* keyword => IF                     */
   0,                                  /* keyword => INHERIT                */
   0,                                  /* keyword => LAMBDA                 */
   0,                                  /* keyword => LOOP                   */
   0,                                  /* keyword => NOT                    */
   0,                                  /* keyword => NULL                   */
   1,                                  /* keyword => OR                     */
   0,                                  /* keyword => OTHERWISE              */
   0,                                  /* keyword => PACKAGE                */
   0,                                  /* keyword => PROCEDURE              */
   0,                                  /* keyword => PROCESS                */
   0,                                  /* keyword => PROGRAM                */
   0,                                  /* keyword => RD                     */
   0,                                  /* keyword => RETURN                 */
   0,                                  /* keyword => RW                     */
   0,                                  /* keyword => SEL                    */
   0,                                  /* keyword => SELF                   */
   0,                                  /* keyword => STOP                   */
   0,                                  /* keyword => THEN                   */
   0,                                  /* keyword => UNTIL                  */
   0,                                  /* keyword => USE                    */
   0,                                  /* keyword => VAR                    */
   0,                                  /* keyword => WHEN                   */
   0,                                  /* keyword => WHILE                  */
   0,                                  /* keyword => WR                     */
   0,                                  /* ;                                 */
   0,                                  /* ,                                 */
   0,                                  /* :                                 */
   0,                                  /* (                                 */
   0,                                  /* )                                 */
   0,                                  /* [                                 */
   0,                                  /* ]                                 */
   0,                                  /* {                                 */
   0,                                  /* }                                 */
   0,                                  /* .                                 */
   0,                                  /* ..                                */
   0,                                  /* :=                                */
   0,                                  /* |                                 */
   0,                                  /* =>                                */
   0,                                  /* assignment operator               */
   0,                                  /* assignment operator               */
   0,                                  /* unary operator                    */
   0,                                  /* pointer reference                 */
   0,                                  /* addop                             */
   0,                                  /* -                                 */
   0,                                  /* mulop                             */
   0,                                  /* **                                */
   0,                                  /* relop                             */
   0,                                  /* fromop                            */
   0,                                  /* quantifier                        */
   0,                                  /* keyword => NATIVE                 */
   0,                                  /* ;                                 */
   0,                                  /* .                                 */
   0,                                  /* (                                 */
   0,                                  /* #                                 */
   0,                                  /* POW                               */
   0,                                  /* ARB                               */
   0,                                  /* DOMAIN                            */
   0,                                  /* RANGE                             */
   1,                                  /* +                                 */
   0,                                  /* +:=                               */
   0,                                  /* +/                                */
   0,                                  /* -:=                               */
   0,                                  /* -/                                */
   1,                                  /* ?                                 */
   0,                                  /* ?:=                               */
   0,                                  /* ?/                                */
   1,                                  /* *                                 */
   0,                                  /* *:=                               */
   0,                                  /* * /                               */
   1,                                  /* /                                 */
   0,                                  /* /:=                               */
   0,                                  /* //                                */
   1,                                  /* MOD                               */
   0,                                  /* MOD:=                             */
   0,                                  /* MOD/                              */
   1,                                  /* MIN                               */
   0,                                  /* MIN:=                             */
   0,                                  /* MIN/                              */
   1,                                  /* MAX                               */
   0,                                  /* MAX:=                             */
   0,                                  /* MAX/                              */
   1,                                  /* WITH                              */
   0,                                  /* WITH:=                            */
   0,                                  /* WITH/                             */
   1,                                  /* LESS                              */
   0,                                  /* LESS:=                            */
   0,                                  /* LESS/                             */
   1,                                  /* LESSF                             */
   0,                                  /* LESSF:=                           */
   0,                                  /* LESSF/                            */
   1,                                  /* NPOW                              */
   0,                                  /* NPOW:=                            */
   0,                                  /* NPOW/                             */
   1,                                  /* =                                 */
   0,                                  /* =:=                               */
   0,                                  /* =/                                */
   1,                                  /* /=                                */
   0,                                  /* /=:=                              */
   0,                                  /* /=/                               */
   1,                                  /* <                                 */
   0,                                  /* <:=                               */
   0,                                  /* </                                */
   1,                                  /* <=                                */
   0,                                  /* <=:=                              */
   0,                                  /* <=/                               */
   1,                                  /* >                                 */
   0,                                  /* >:=                               */
   0,                                  /* >/                                */
   1,                                  /* >=                                */
   0,                                  /* >=:=                              */
   0,                                  /* >=/                               */
   1,                                  /* IN                                */
   0,                                  /* IN:=                              */
   0,                                  /* IN/                               */
   1,                                  /* NOTIN                             */
   0,                                  /* NOTIN:=                           */
   0,                                  /* NOTIN/                            */
   1,                                  /* SUBSET                            */
   0,                                  /* SUBSET:=                          */
   0,                                  /* SUBSET/                           */
   1,                                  /* INCS                              */
   0,                                  /* INCS:=                            */
   0,                                  /* INCS/                             */
   0,                                  /* AND:=                             */
   0,                                  /* AND/                              */
   0,                                  /* OR:=                              */
   0,                                  /* OR/                               */
   0,                                  /* FROM                              */
   0,                                  /* FROMB                             */
   0,                                  /* FROME                             */
   0,                                  /* EXISTS                            */
   0,                                  /* FORALL                            */
/* ## end is_binop */
   0};
unsigned char *s,*t;                   /* temporary looping variables       */

   /* we don't have a binary operator yet */

   saving_binop = NO;

   /* loop until we explicitly return */

   for (;;) {

      /* skip white space */

      while (is_white_space(*start)) {

         advance_la;
         start = lookahead;

      }

      switch (*start) {

/*\
 *  \case{newlines}
 *
 *  We have to separate newlines from other whitespace, to keep track of
 *  line numbers.
\*/

case '\n' :

{

   curr_line++;
   advance_la;
   start = lookahead;

   while (*start == '\r' && *start != EOFCHAR) {

         advance_la;
         start = lookahead;

      }

   curr_line_start = start;
   curr_col_adjustment = 0;
   break;

}

/*\
 *  \case{carriage returns}
 *
 *  Carriage returns are whitespace.
\*/

case '\r' :

{

   curr_line++;
   advance_la;
   start = lookahead;

   while (*start == '\n' && *start != EOFCHAR) {

         advance_la;
         start = lookahead;

      }

   curr_line_start = start;
   curr_col_adjustment = 0;
   break;

}

/*\
 *  \case{tabs}
 *
 *  We have to separate tabs from other whitespace, to keep accurate
 *  column numbers.
\*/

case '\t' :

{

   advance_la;
   start = lookahead;
   while ((lookahead-curr_line_start+1+curr_col_adjustment) % TAB_WIDTH != 1)
      curr_col_adjustment++;
   break;

}

/*\
 *  \case{backspaces}
 *
 *  We have to separate backaspaces from other whitespace, to keep accurate
 *  column numbers.
\*/

case 8 :

{

   advance_la;
   start = lookahead;
   curr_col_adjustment--;
   break;

}

/*\
 *  \case{comments}
 *
 *  Comments start with \verb"--", and extend to the end of the line.  We
 *  must also handle minus signs in this case.
\*/

case '-' :
/* Syntax extension */

case 225:  /* shift-option-9 */
case 126:  /* ~ */
case 33:  /* ! */
case 64:  /* @ */
case 36:  /* $ */
case 37:  /* % */
case 38:  /* & */
case 92:  /* \ */
case 39:  /* ' */
case 163:  /* option-3 */
case 162:  /* option-4 */
case 176:  /* option-5 */
case 164:  /* option-6 */
case 166:  /* option-7 */
case 165:  /* option-8 */
case 186:  /* option-b */
case 182:  /* option-d */
case 198:  /* option-j */
case 194:  /* option-l */
case 181:  /* option-m */
case 191:  /* option-o */
case 185:  /* option-p */
case 167:  /* option-s */
case 160:  /* option-t */
case 183:  /* option-w */
case 197:  /* option-x */
case 180:  /* option-y */
case 189:  /* option-z */
case 224:  /* shift-option-7 */
case 177:  /* shift-option-= */
case 184:  /* shift-option-P */
case 215:  /* shift-option-V */

 

{
namtab_ptr_type id_ptr;                /* id entry in name table            */

   /* check for comments */
   
   advance_la;
   if ((*start=='-')&&(*lookahead == '-')) {

	      while (*start != '\n' && *start!= '\r' && *start != EOFCHAR) {

	         advance_la;
	         start = lookahead;

	      }

	      break;

   }
   
   /* if we're saving a binary operator, return */

   if (saving_binop) {
      lookahead = start;
      saving_binop = NO;
      return;
   }
   
   /* */
   
   if (*start!='-')  {
   	  int k;
   		
	   while (is_id_char(*lookahead) && (*lookahead>'9') && lookahead - start < MAX_TOK_LEN)
	      advance_la;
	  	
	  	
	   /* copy the identifier to the lexeme buffer */
	   strcpy(lexeme,extension_map[*start]);
	   k=strlen(lexeme);
	   lexeme[k]='_';
	   
	   for (s = start+1, t = (unsigned char *)lexeme+k+1; s < lookahead; s++)
	      *t++ = to_upper(*s);
	   *t = '\0';

	   /* report an error if the lexeme is too long */

	   if (lookahead - start >= MAX_TOK_LEN) {

	      /* report the error */

	      lexeme_pos.fp_line = curr_line;
	      lexeme_pos.fp_column = start - curr_line_start + 1;
	      error_message(SETL_SYSTEM &lexeme_pos,msg_token_too_long,lexeme);

	      build_token(tok_error,tok_error,nam_error);

	      /* position start past the bad lexeme */

	      start = lookahead;
	      while (is_id_char(*start)) {
	         advance_la;
	         start = lookahead;
	      }

	      /* return an error token */

	      return;

	   }

	   /* look up the lexeme in the name table */

	   id_ptr = get_namtab(SETL_SYSTEM lexeme);

		   
	   if (id_ptr->nt_token_class == -1) {
	      id_ptr->nt_token_class = tok_id;
	      id_ptr->nt_token_subclass = tok_id;
	   }
	   
	  

	   /* build and save a token */

	   build_token(tok_dash,tok_dash,id_ptr);
	   start = lookahead;

	   saving_binop = YES;
	   break;
   
   }
   
   


   /* build and save a token */

   build_token(tok_dash,tok_dash,nam_dash);
   start = lookahead;

   saving_binop = YES;
   break;

}

/*\
 *  \case{reserved words and identifiers}
 *
 *  Reserved words and identifiers follow the same lexical rules. Reserved
 *  words are simply placed in the name table on initialization with their
 *  token classes.  In this case, we read an identifier and look it up in
 *  the name table.  If we find it, we return the token class and subclass
 *  we find.  Otherwise, we install it as an identifier.
\*/

case 'a': case 'b': case 'c': case 'd': case 'e': case 'f' : case 'g' :
case 'h': case 'i': case 'j': case 'k': case 'l': case 'm' : case 'n' :
case 'o': case 'p': case 'q': case 'r': case 's': case 't' : case 'u' :
case 'v': case 'w': case 'x': case 'y': case 'z':
case 'A': case 'B': case 'C': case 'D': case 'E': case 'F' : case 'G' :
case 'H': case 'I': case 'J': case 'K': case 'L': case 'M' : case 'N' :
case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T' : case 'U' :
case 'V': case 'W': case 'X': case 'Y': case 'Z':

{
namtab_ptr_type id_ptr;                /* id entry in name table            */

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* find the end of the identifier */

   lookahead = start;
   while (is_id_char(*lookahead) && lookahead - start < MAX_TOK_LEN)
      advance_la;

   /* copy the identifier to the lexeme buffer */

   for (s = start, t = (unsigned char *)lexeme; s < lookahead; s++)
      *t++ = to_upper(*s);
   *t = '\0';

   /* report an error if the lexeme is too long */

   if (lookahead - start >= MAX_TOK_LEN) {

      /* report the error */

      lexeme_pos.fp_line = curr_line;
      lexeme_pos.fp_column = start - curr_line_start + 1;
      error_message(SETL_SYSTEM &lexeme_pos,msg_token_too_long,lexeme);

      build_token(tok_error,tok_error,nam_error);

      /* position start past the bad lexeme */

      start = lookahead;
      while (is_id_char(*start)) {
         advance_la;
         start = lookahead;
      }

      /* return an error token */

      return;

   }

   /* look up the lexeme in the name table */

   id_ptr = get_namtab(SETL_SYSTEM lexeme);

   /* if we didn't find it, flag it as an identifier */

   if (id_ptr->nt_token_class == -1) {
      id_ptr->nt_token_class = tok_id;
      id_ptr->nt_token_subclass = tok_id;
   }

   build_token(id_ptr->nt_token_class,id_ptr->nt_token_subclass,id_ptr);

   start = lookahead;

   /* if we found a binary operator, save it */

   if (is_binop[id_ptr->nt_token_subclass]) {

      saving_binop = YES;
      break;

   }

   return;

}

/*\
 *  \case{numeric literals}
 *
 *  This case handles numeric literals, both integer and real.  Numeric
 *  literals in SETL2 borrow ideas from Ada, Icon, and SETL. Like SETL,
 *  integers can be infinite in length during execution, but literals are
 *  limited by the maximum length of a lexical token.  Like Ada, we use
 *  the pound sign, \verb"#", to delimit base changes.  Like Icon, we
 *  allow numbers to use any base from 2 to 36, using alphabetic
 *  characters to represent the digits 10 to 35.
 *
 *  The lexical analyzer is only responsible for scanning and error
 *  checking.  We do not convert numbers to internal form here.
\*/

case '0': case '1': case '2': case '3': case '4': case '5': case '6':
case '7': case '8': case '9':

{
char error_text[100];                  /* error message text                */
int lex_error;                         /* YES => error, NO => no error      */
int base;                              /* numeric base                      */
int special_base;                      /* YES if we had a base change       */
int is_real;                           /* YES => real, NO => integer        */
namtab_ptr_type namtab_ptr;            /* pointer to installed name         */
symtab_ptr_type symtab_ptr;            /* pointer to installed symbol       */
char *t;                               /* used to loop over lexeme          */

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   base = 10;
   special_base = NO;
   is_real = NO;
   lex_error = NO;
   t = lexeme;

   /* first we find either the base or whole part */

   while (is_digit(*lookahead,10) || *lookahead == '_') {

      if (t < lexeme + MAX_TOK_LEN) {
         *t++ = *lookahead;
         advance_la;
      }
      else {
         advance_la;
         start = lookahead;
      }
   }

   /* we found a base, pick up the whole part */

   if (*lookahead == '#') {

      if (t < lexeme + MAX_TOK_LEN)
         *t++ = *lookahead;
      advance_la;

      /* we need to use the base to determine if characters are digits */

      special_base = YES;
      base = 0;
      for (t = lexeme; *t != '#'; t++)
         base = base * 10 + *t - '0';
      t++;

      /* we allow bases from 2 to 36 */

      if (base < 2 || base > 36) {
         if (!lex_error) {
            strcpy(error_text,"Invalid base");
            lex_error = YES;
         }
         base = 36;
      }

      /* now we know we have a whole number */

      while (is_digit(*lookahead,base) || *lookahead == '_') {

         if (t < lexeme + MAX_TOK_LEN) {
            *t++ = *lookahead;
            advance_la;
         }
         else {
            advance_la;
            start = lookahead;
         }
      }
   }

   /* if we have a decimal point, we have a real literal */

   advance_la;
   lookahead--;
   if (*lookahead == '.' && is_digit(*(lookahead + 1),base)) {

      is_real = YES;
      if (t < lexeme + MAX_TOK_LEN)
         *t++ = *lookahead;
      advance_la;

      /* find the decimal part of the real number */

      while (is_digit(*lookahead,base) || *lookahead == '_') {

         if (t < lexeme + MAX_TOK_LEN) {
            *t++ = *lookahead;
            advance_la;
         }
         else {
            advance_la;
            start = lookahead;
         }
      }
   }

   /* if we have a special base, we expect a '#' at this point */

   if (special_base) {

      if (*lookahead != '#') {

         if (!lex_error) {
            strcpy(error_text,"Expected #");
            lex_error = YES;
         }
      }
      else {

         *t++ = *lookahead;
         advance_la;

      }
   }

   if (is_real) {

      /* check for an exponent */

      if (*lookahead == 'e' || *lookahead == 'E') {

         if (t < lexeme + MAX_TOK_LEN)
            *t++ = *lookahead;
         advance_la;

         if (*lookahead == '+' || *lookahead == '-') {

            if (t < lexeme + MAX_TOK_LEN)
               *t++ = *lookahead;
            advance_la;

         }

         if (t < lexeme + MAX_TOK_LEN)
            *t++ = *lookahead;
         advance_la;

         while (is_digit(*lookahead,base) || *lookahead == '_') {

            if (t < lexeme + MAX_TOK_LEN) {
               *t++ = *lookahead;
               advance_la;
            }
            else {
               advance_la;
               start = lookahead;
            }
         }
      }
   }

   *t = '\0';

   /* if we've exceeded the maximum token length, report an error */

   lexeme_pos.fp_line = curr_line;
   lexeme_pos.fp_column = start - curr_line_start + 1;
   if (t >= lexeme + MAX_TOK_LEN) {

      /* report the error */

      error_message(SETL_SYSTEM &lexeme_pos,msg_token_too_long,lexeme);

      build_token(tok_error,tok_error,nam_error);

      /* return an error token */

      return;

   }

   /* look up the lexeme in the name table */

   namtab_ptr = get_namtab(SETL_SYSTEM lexeme);

   /* if we don't find it, make a literal item */

   if (namtab_ptr->nt_symtab_ptr == NULL) {

      namtab_ptr->nt_token_class = tok_literal;
      symtab_ptr = enter_symbol(SETL_SYSTEM
                                namtab_ptr,
                                unit_proctab_ptr,
                                &(token->tk_file_pos));
      symtab_ptr->st_has_rvalue = YES;
      symtab_ptr->st_is_initialized = YES;

      if (is_real) {

         namtab_ptr->nt_token_subclass = tok_real;
         symtab_ptr->st_type = sym_real;
         symtab_ptr->st_aux.st_real_ptr = char_to_real(SETL_SYSTEM
                                                       lexeme,&lexeme_pos);

      }
      else {

         namtab_ptr->nt_token_subclass = tok_integer;
         symtab_ptr->st_type = sym_integer;
         symtab_ptr->st_aux.st_integer_ptr = char_to_int(SETL_SYSTEM lexeme);

      }
   }

   /* that's it --- build up a token and return */

   build_token(namtab_ptr->nt_token_class,
               namtab_ptr->nt_token_subclass,
               namtab_ptr);

   start = lookahead;

   return;

}

/*\
 *  \case{string literals}
 *
 *  The definition of string literals was taken from C, except for the
 *  concept of a null-terminated string.  Strings do not necessarily stop
 *  at a null.
 *
 *  Like numeric literals, we do not convert to the internal form of a
 *  string here.
\*/

case '\"' :

{
char error_text[100];                  /* error message text                */
int lex_error;                         /* YES => error, NO => no error      */
namtab_ptr_type namtab_ptr;            /* name table pointer                */
symtab_ptr_type symtab_ptr;            /* symbol table pointer              */

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   lookahead = start;
   advance_la;
   lex_error = NO;

   /* scan the string, verifying escape sequences */

   for (;;) {

      if (*lookahead == '\"') {
         advance_la;
         break;
      }

      /* check for token too long */

      if (lookahead > start + MAX_TOK_LEN) {

         if (!lex_error) {
            strcpy(error_text,"String literal exceeds maximum token length");
            lex_error = YES;
         }

         start = lookahead;

         break;

      }

      /* check for unterminated literal */

      if (*lookahead == '\r' ||
          *lookahead == '\n' ||
          *lookahead == EOFCHAR) {

         if (!lex_error) {
            strcpy(error_text,"Unterminated string literal");
            lex_error = YES;
         }
         break;

      }

      /* check escape sequences */

      if (*lookahead == '\\') {

         advance_la;

         switch (*lookahead) {

            case '\\' :
               break;

            case '0' :
               break;

            case 'n' :
               break;

            case 'r' :
               break;

            case 'f' :
               break;

            case 't' :
               break;

            case '\"' :
               break;

            case 'x' : case 'X' :

               advance_la;
               advance_la;

               if (!is_digit(*(lookahead-1),16) ||
                   !is_digit(*lookahead,16)) {

                  if (!lex_error) {
                     sprintf(error_text,
                             "Invalid hex character => %c%c",
                             *(lookahead-1),
                             *lookahead);
                     lex_error = YES;
                  }

                  lookahead--;

               }

               break;

            default :

               if (!lex_error) {
                  strcpy(error_text,"Invalid escape sequence");
                  lex_error = YES;
               }

         }

         advance_la;
         continue;

      }

      advance_la;

   }

   /* copy buffer to lexeme */

   for (s = start, t = (unsigned char *)lexeme; s < lookahead; *t++ = *s++);
   *t = '\0';

   /* if we found an error, report it */

   if (lex_error) {

      lexeme_pos.fp_line = curr_line;
      lexeme_pos.fp_column = start - curr_line_start + 1;
      error_message(SETL_SYSTEM &lexeme_pos,"%s => %s",error_text,lexeme);

      build_token(tok_error,tok_error,nam_error);

      start = lookahead;

      return;

   }

   /* look up the lexeme in the name table */

   namtab_ptr = get_namtab(SETL_SYSTEM lexeme);

   /* if we didn't find it, build a literal item */

   if (namtab_ptr->nt_symtab_ptr == NULL) {

      namtab_ptr->nt_token_class = tok_literal;
      namtab_ptr->nt_token_subclass = tok_string;
      symtab_ptr = enter_symbol(SETL_SYSTEM
                                namtab_ptr,
                                unit_proctab_ptr,
                                &(token->tk_file_pos));

      symtab_ptr->st_type = sym_string;
      symtab_ptr->st_has_rvalue = YES;
      symtab_ptr->st_is_initialized = YES;
      symtab_ptr->st_aux.st_string_ptr = char_to_string(SETL_SYSTEM lexeme);

   }

   /* that's it --- build a token and return */

   build_token(tok_literal,tok_string,namtab_ptr);

   start = lookahead;

   return;

}

/*\
 *  \case{special symbols}
 *
 *  The following cases handle special symbols made up of non-alphanumeric
 *  characters.
\*/

case ';' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_semi,tok_semi,nam_semi);
   advance_la;
   start = lookahead;

   return;

}

case ',' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_comma,tok_comma,nam_comma);
   advance_la;
   start = lookahead;

   return;

}

case ':' :

{
namtab_ptr_type namtab_ptr;            /* pointer to composite operator     */

   /* check for assignment symbol (:=) */

   advance_la;
   if (*lookahead == '=') {

      /* check for assignment operator */

      if (saving_binop) {

         saving_binop = NO;
         strcpy(lexeme,(token->tk_namtab_ptr)->nt_name);
         strcat(lexeme,":=");
         namtab_ptr = get_namtab(SETL_SYSTEM lexeme);

         if (namtab_ptr->nt_token_class != -1) {

            token->tk_token_class = namtab_ptr->nt_token_class;
            token->tk_token_subclass = namtab_ptr->nt_token_subclass;
            token->tk_namtab_ptr = namtab_ptr;
            advance_la;
            start = lookahead;

            return;

         }
      }

      /* build and return a token */

      build_token(tok_assign,tok_assign,nam_assign);
      advance_la;
      start = lookahead;

      return;

   }

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_colon,tok_colon,nam_colon);
   start = lookahead;

   return;

}

case '|' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_suchthat,tok_suchthat,nam_suchthat);
   advance_la;
   start = lookahead;

   return;

}

case '(' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_lparen,tok_lparen,nam_lparen);
   advance_la;
   start = lookahead;

   return;

}

case ')' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_rparen,tok_rparen,nam_rparen);
   advance_la;
   start = lookahead;

   return;

}

case '[' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_lbracket,tok_lbracket,nam_lbracket);
   advance_la;
   start = lookahead;

   return;

}

case ']' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_rbracket,tok_rbracket,nam_rbracket);
   advance_la;
   start = lookahead;

   return;

}

case '{' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_lbrace,tok_lbrace,nam_lbrace);
   advance_la;
   start = lookahead;

   return;

}

case '}' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_rbrace,tok_rbrace,nam_rbrace);
   advance_la;
   start = lookahead;

   return;

}

case '.' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   advance_la;
   if (*lookahead == '.') {

      build_token(tok_dotdot,tok_dotdot,nam_dotdot);
      advance_la;
      start = lookahead;

      return;

   }

   build_token(tok_dot,tok_dot,nam_dot);
   start = lookahead;

   return;

}

case '+' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and save a token */

   build_token(tok_addop,tok_plus,nam_plus);
   advance_la;
   start = lookahead;

   saving_binop = YES;
   break;

}

case '#' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_unop,tok_nelt,nam_nelt);
   advance_la;
   start = lookahead;

   return;

}

case '^' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return a token */

   build_token(tok_caret,tok_caret,nam_caret);
   advance_la;
   start = lookahead;

   return;

}

case '*' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and save a token */

   advance_la;
   if (*lookahead == '*') {

      build_token(tok_expon,tok_expon,nam_expon);
      advance_la;
      start = lookahead;

      saving_binop = YES;
      break;

   }

   build_token(tok_mulop,tok_mult,nam_mult);
   start = lookahead;

   saving_binop = YES;
   break;

}

case '?' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and save a token */

   build_token(tok_mulop,tok_question,nam_question);
   advance_la;
   start = lookahead;

   saving_binop = YES;
   break;

}

case '/' :

{
namtab_ptr_type namtab_ptr;            /* pointer to composite operator     */

   /* check for application operator */

   if (saving_binop) {

      saving_binop = NO;
      strcpy(lexeme,(token->tk_namtab_ptr)->nt_name);
      strcat(lexeme,"/");
      namtab_ptr = get_namtab(SETL_SYSTEM lexeme);

      if (namtab_ptr->nt_token_class != -1) {

         token->tk_token_class = namtab_ptr->nt_token_class;
         token->tk_token_subclass = namtab_ptr->nt_token_subclass;
         token->tk_namtab_ptr = namtab_ptr;
         advance_la;
         start = lookahead;

         return;

      }
   }

   /* build and return a token */

   advance_la;
   if (*lookahead == '=') {

      build_token(tok_relop,tok_ne,nam_ne);
      advance_la;
      start = lookahead;

      saving_binop = YES;
      break;

   }

   build_token(tok_mulop,tok_slash,nam_slash);
   start = lookahead;

   saving_binop = YES;
   break;

}

case '=' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and return or save token */

   advance_la;
   if (*lookahead == '>') {

      build_token(tok_rarrow,tok_rarrow,nam_rarrow);
      advance_la;
      start = lookahead;

      return;

   }

   build_token(tok_relop,tok_eq,nam_eq);
   start = lookahead;

   saving_binop = YES;
   break;

}

case '<' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and save a token */

   advance_la;
   if (*lookahead == '=') {

      build_token(tok_relop,tok_le,nam_le);
      advance_la;
      start = lookahead;

      saving_binop = YES;
      break;

   }

   build_token(tok_relop,tok_lt,nam_lt);
   start = lookahead;

   saving_binop = YES;
   break;

}

case '>' :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* build and save a token */

   advance_la;
   if (*lookahead == '=') {

      build_token(tok_relop,tok_ge,nam_ge);
      advance_la;
      start = lookahead;

      saving_binop = YES;
      break;

   }

   build_token(tok_relop,tok_gt,nam_gt);
   start = lookahead;

   saving_binop = YES;
   break;

}

/*\
 *  \case{end of file}
 *
 *  When we see the end of file character, we just return it and do NOT
 *  advance the pointers.  If \verb"get_token()" is called again, it will
 *  return \verb"tok_eof" again.
\*/

case EOFCHAR :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   build_token(tok_eof,tok_eof,nam_eof);

   return;

}

/*\
 *  \case{errors}
 *
 *  If we get here, we must have a lexical error.
\*/

default :

{

   /* if we're saving a binary operator, return */

   if (saving_binop) {
      saving_binop = NO;
      return;
   }

   /* return an error token */

   build_token(tok_error,tok_error,nam_error);
   error_message(SETL_SYSTEM &lexeme_pos,"Invalid lexical token => %c",*start);
   advance_la;
   start = lookahead;
   return;

}

/* back to normal indentation */

      }
   }

   return;

}

/*\
 *  \function{fill\_buffer()}
 *
 *  This function loads the source buffer from the input file.  First we
 *  shift the current buffer from the start of the current token to the
 *  front of the source buffer.  We then read from the source file to the
 *  lookahead pointer.  We never lose part of the current token by
 *  overwriting it.
 *
 *  This scheme works reasonably efficiently provided the buffer size is
 *  considerably longer than the average token size.  We don't see why
 *  this would not be the case.
\*/

static void fill_buffer(SETL_SYSTEM_PROTO_VOID)

{
int readcount;                         /* number of characters read         */
unsigned char *s,*t;                   /* temporary looping variables       */

#ifdef TRAPS

   if (lookahead - start > MAX_TOK_LEN)
      giveup(SETL_SYSTEM "Compiler error -- token too long discovered in fill_buffer()");

#endif

   /* shift the current token to the start of the source_buffer */

   curr_line_start = source_buffer + (curr_line_start - start);
   for (t = source_buffer, s = start; s < lookahead; *t++ = *s++);
   start = source_buffer;
   lookahead = t;

   /* read a block starting at the lookahead pointer */

#ifdef DYNAMIC_COMP
   readcount=strlen(PROGRAM_FRAGMENT);
   if (readcount > CHAR_BUFF_SIZE)
        readcount=CHAR_BUFF_SIZE;
   if (readcount>0)
        strncpy((char *)lookahead,PROGRAM_FRAGMENT,readcount);
        PROGRAM_FRAGMENT+=readcount;
#else
   if ((readcount = fread((void *)lookahead,
                          (size_t)1,
                          (size_t)CHAR_BUFF_SIZE,
                          SOURCE_FILE)) < 0)
      giveup(SETL_SYSTEM "Disk error reading %s",SOURCE_NAME);

   if (ferror(SOURCE_FILE) != 0)
      giveup(SETL_SYSTEM "Disk error reading %s",SOURCE_NAME);
#endif

   /* adjust the end of buffer pointer */

   if (readcount == 0) {
      *t = EOFCHAR;
      endofbuffer = lookahead;
   }
   else {
      endofbuffer = lookahead + readcount - 1;
   }

   return;

}

