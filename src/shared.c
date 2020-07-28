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
 *  \package{Shared Data}
 *
 *  This source file is really a dummy package, which contains any shared
 *  data in all other packages.  In the header file associated with any
 *  other package there are two copies of each declaration.  The first is
 *  used if the symbol SHARED is defined.  The second declares the item to
 *  be external.  This package is the only one which has SHARED defined,
 *  so the shared data from all other packages will be in the {\tt .obj}
 *  file associated with this source file.
\*/

#define SHARED 1

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "builtins.h"                  /* built-in symbols                  */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "chartab.h"                   /* character type table              */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "ast.h"                       /* abstract syntax tree              */
#include "quads.h"                     /* quadruples                        */
#include "lex.h"                       /* lexical analyzer                  */
#include "parsetab.h"                  /* parse tables                      */
#include "genquads.h"                  /* quadruple generator               */
#include "genstmt.h"                   /* statement code generator          */
#include "genexpr.h"                   /* expression code generator         */

#include "giveup.h"                    /* severe error handler              */
#include "mcode.h"                     /* method codes                      */
#include "pcode.h"                     /* pseudo code                       */
#include "filename.h"                  /* file name utilities               */
#include "libman.h"                    /* library manager                   */
#include "unittab.h"                   /* unit table                        */
#include "loadunit.h"                  /* unit loader                       */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "sets.h"                      /* sets                              */
#include "maps.h"                      /* maps                              */
#include "tuples.h"                    /* tuples                            */
#include "procs.h"                     /* procedures                        */
#include "objects.h"                   /* objects                           */
#include "process.h"                   /* objects                           */
#include "mailbox.h"                   /* objects                           */
#include "slots.h"                     /* procedures                        */
#include "x_files.h"                   /* files                             */
#include "iters.h"                     /* iterators                         */
#include "axobj.h"

/* Pull in the header files defined both in the compiler and the 
   interpreter ... */

#include "c_integers.h"             /* integers                          */
#include "x_integers.h"             /* integers                          */
#include "c_reals.h"                /* real numbers                      */
#include "x_reals.h"                /* real numbers                      */
#include "c_strngs.h"               /* strings                           */
#include "x_strngs.h"               /* strings                           */

#ifdef TSAFE
#include "shared.h"
#endif

