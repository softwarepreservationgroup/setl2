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
 *  \package{Command Line}
 *
 *  This function returns a character string of options which are placed
 *  in front of the command line.  It simply finds the appropriate
 *  environment string and returns it.  It is a separate function so that
 *  it can be overridden when interpreters are built using callout which
 *  perform specific programs.
\*/

/* standard C header files */

#include "system.h"                    /* SETL2 system constants            */

#include <ctype.h>                     /* character macros                  */
#ifdef HAVE_SIGNAL_H
#include <signal.h>                    /* signal macros                     */
#endif

/* SETL2 system header files */

#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "cmdline.h"                   /* command line argument string      */

/*\
 *  \function{cmdline()}
 *
 *  This function just reads an environment string and returns it.  We do
 *  this in a separate function so that it can be easily overridden to
 *  create a custom interpreter for a specific program.
\*/

char *cmdline()
{
    return getenv(INTERP_OPTIONS_KEY);
}
