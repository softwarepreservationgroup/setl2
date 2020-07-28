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
 *  \package{Constant Check}
 *
 *  This package simply checks whether an expression is constant.  It is
 *  used by the case code generation routines to determine whether the
 *  expressions in a case statement are constant.
 *
 *  \texify{const.h}
 *
 *  \packagebody{Constant Check}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "symtab.h"                    /* symbol table                      */
#include "ast.h"                       /* abstract syntax tree              */

/*\
 *  \function{is\_constant()}
 *
 *  We return YES if the ast is a constant symbol or if it is a simple
 *  expression all of whose operands are constant.
\*/

int is_constant(
   ast_ptr_type root)                  /* AST to be checked                 */

{

   switch (root->ast_type) {

case ast_symtab :

{
symtab_ptr_type symtab_ptr;            /* symbol table pointer from ast     */

   symtab_ptr = root->ast_child.ast_symtab_ptr;

   return (symtab_ptr->st_has_rvalue && !symtab_ptr->st_has_lvalue);

}

default :

{
   return NO;
}

/* at this point we return to normal indentation */

}}
