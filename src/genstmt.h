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
 *  \packagespec{Statement Code Generator}
\*/

#ifndef GENSTMT_LOADED

/*\
 *  \table{statement generator functions}
\*/

/* statement generator type definition */

typedef void ((*gen_stmt_func)(SETL_SYSTEM_PROTO struct ast_item *));

#ifdef SHARED

void gen_stmt_null(SETL_SYSTEM_PROTO ast_ptr_type);      
                                       /* null tree                         */
void gen_stmt_list(SETL_SYSTEM_PROTO ast_ptr_type);      
                                       /* statement lists                   */
void gen_stmt_symtab(SETL_SYSTEM_PROTO ast_ptr_type);    
                                       /* symbol table pointers             */
void gen_stmt_assign(SETL_SYSTEM_PROTO ast_ptr_type);    
                                       /* assignment statements             */
void gen_stmt_assignop(SETL_SYSTEM_PROTO ast_ptr_type);  
                                       /* assignment operators              */
void gen_stmt_if(SETL_SYSTEM_PROTO ast_ptr_type);        
                                       /* if statement                      */
void gen_stmt_while(SETL_SYSTEM_PROTO ast_ptr_type);     
                                       /* while statement                   */
void gen_stmt_until(SETL_SYSTEM_PROTO ast_ptr_type);     
                                       /* until statement                   */
void gen_stmt_loop(SETL_SYSTEM_PROTO ast_ptr_type);      
                                       /* loop statement                    */
void gen_stmt_for(SETL_SYSTEM_PROTO ast_ptr_type);       
                                       /* for statement                     */
void gen_stmt_case(SETL_SYSTEM_PROTO ast_ptr_type);      
                                       /* case statment                     */
void gen_stmt_guard(SETL_SYSTEM_PROTO ast_ptr_type);
void gen_stmt_call(SETL_SYSTEM_PROTO ast_ptr_type);      
                                       /* procedure calls                   */
void gen_stmt_return(SETL_SYSTEM_PROTO ast_ptr_type);
void gen_stmt_stop(SETL_SYSTEM_PROTO ast_ptr_type);
void gen_stmt_exit(SETL_SYSTEM_PROTO ast_ptr_type);
void gen_stmt_continue(SETL_SYSTEM_PROTO ast_ptr_type);
void gen_stmt_assert(SETL_SYSTEM_PROTO ast_ptr_type);
void gen_stmt_error(SETL_SYSTEM_PROTO ast_ptr_type);     
                                       /* error node                        */
void gen_stmt_from(SETL_SYSTEM_PROTO ast_ptr_type);      
                                       /* error node                        */
void gen_stmt_slot(SETL_SYSTEM_PROTO ast_ptr_type);      
                                       /* error node                        */
void gen_stmt_initobj(SETL_SYSTEM_PROTO ast_ptr_type);   
                                       /* error node                        */
void gen_stmt_slotof(SETL_SYSTEM_PROTO ast_ptr_type);    
                                       /* error node                        */

gen_stmt_func gen_stmt_table[] = {     /* statement generator functions     */
/* ## begin gen_stmt_table */
   gen_stmt_null,                      /* null tree                         */
   gen_stmt_list,                      /* statement or expression list      */
   gen_stmt_error,                     /* name table pointer                */
   gen_stmt_symtab,                    /* symbol table pointer              */
   gen_stmt_error,                     /* name qualifier                    */
   gen_stmt_error,                     /* +                                 */
   gen_stmt_error,                     /* -                                 */
   gen_stmt_error,                     /* *                                 */
   gen_stmt_error,                     /* /                                 */
   gen_stmt_error,                     /* **                                */
   gen_stmt_error,                     /* MOD                               */
   gen_stmt_error,                     /* MIN                               */
   gen_stmt_error,                     /* MAX                               */
   gen_stmt_error,                     /* ?                                 */
   gen_stmt_error,                     /* with operator                     */
   gen_stmt_error,                     /* less operator                     */
   gen_stmt_error,                     /* lessf operator                    */
   gen_stmt_error,                     /* npow operator                     */
   gen_stmt_error,                     /* unary minus                       */
   gen_stmt_error,                     /* unary from                        */
   gen_stmt_error,                     /* map domain                        */
   gen_stmt_error,                     /* map range                         */
   gen_stmt_error,                     /* not                               */
   gen_stmt_error,                     /* arb                               */
   gen_stmt_error,                     /* pow                               */
   gen_stmt_error,                     /* #                                 */
   gen_stmt_error,                     /* string, map, or tuple component   */
   gen_stmt_error,                     /* multi-valued map `of'             */
   gen_stmt_error,                     /* string, map, or tuple component   */
                                       /* kill temp after assignment        */
   gen_stmt_error,                     /* multi-valued map `of'             */
                                       /* kill temp after assignment        */
   gen_stmt_error,                     /* string or tuple slice             */
   gen_stmt_error,                     /* string or tuple tail              */
   gen_stmt_assign,                    /* general assignment                */
   gen_stmt_assignop,                  /* assignment operators              */
   gen_stmt_error,                     /* procedure with environment        */
   gen_stmt_error,                     /* constant initialization           */
   gen_stmt_error,                     /* place holder in tuple lhs         */
   gen_stmt_from,                      /* from operator                     */
   gen_stmt_from,                      /* fromb operator                    */
   gen_stmt_from,                      /* frome operator                    */
   gen_stmt_error,                     /* =                                 */
   gen_stmt_error,                     /* /=                                */
   gen_stmt_error,                     /* <                                 */
   gen_stmt_error,                     /* <=                                */
   gen_stmt_error,                     /* >                                 */
   gen_stmt_error,                     /* >=                                */
   gen_stmt_error,                     /* in                                */
   gen_stmt_error,                     /* notin                             */
   gen_stmt_error,                     /* in                                */
   gen_stmt_error,                     /* subset                            */
   gen_stmt_error,                     /* or operator                       */
   gen_stmt_error,                     /* and operator                      */
   gen_stmt_error,                     /* enumerated set                    */
   gen_stmt_error,                     /* enumerated tuple                  */
   gen_stmt_error,                     /* general set former                */
   gen_stmt_error,                     /* general tuple former              */
   gen_stmt_error,                     /* general set former without        */
                                       /* expression                        */
   gen_stmt_error,                     /* general tuple former without      */
                                       /* expression                        */
   gen_stmt_error,                     /* arithmetic set former             */
   gen_stmt_error,                     /* arithmetic tuple former           */
   gen_stmt_error,                     /* exists expression                 */
   gen_stmt_error,                     /* forall expression                 */
   gen_stmt_error,                     /* application over set              */
   gen_stmt_error,                     /* binary application over set       */
   gen_stmt_error,                     /* iterator list                     */
   gen_stmt_error,                     /* exists iterator list              */
   gen_stmt_if,                        /* if statement                      */
   gen_stmt_error,                     /* if expression                     */
   gen_stmt_loop,                      /* loop statement                    */
   gen_stmt_while,                     /* while statement                   */
   gen_stmt_until,                     /* until statement                   */
   gen_stmt_for,                       /* for statement                     */
   gen_stmt_case,                      /* case statement                    */
   gen_stmt_error,                     /* case expression                   */
   gen_stmt_guard,                     /* guard statement                   */
   gen_stmt_error,                     /* guard expression                  */
   gen_stmt_error,                     /* when clause                       */
   gen_stmt_call,                      /* procedure call                    */
   gen_stmt_return,                    /* return statement                  */
   gen_stmt_stop,                      /* stop statement                    */
   gen_stmt_exit,                      /* break out of loop                 */
   gen_stmt_continue,                  /* continue loop                     */
   gen_stmt_assert,                    /* assert expressions                */
   gen_stmt_error,                     /* initialize object                 */
   gen_stmt_slot,                      /* slot reference                    */
   gen_stmt_slotof,                    /* call slot reference               */
   gen_stmt_slotof,                    /* call slot reference               */
   gen_stmt_error,                     /* method with environment           */
                                       /* or instance                       */
   gen_stmt_error,                     /* self reference                    */
/* ## end gen_stmt_table */
   NULL};

#else

extern gen_stmt_func gen_stmt_table[]; /* statement generator functions     */

#endif

/* gen_statement macro */

#define gen_statement(r) (*(gen_stmt_table[(r)->ast_type]))(r)

#define GENSTMT_LOADED 1
#endif
