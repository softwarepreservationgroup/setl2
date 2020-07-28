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
 *  \package{Form Types}
 *
 *  In SETL2, like SETL, we use the term `forms' to describe what would
 *  commonly be called `types'.  Type is a little too restrictive for SETL2
 *  objects, because they can be overloaded.  For example, as set of
 *  integers has a different type as a set of real numbers, but it has
 *  the same form.  This package contains form codes and descriptions.
\*/

#ifndef FORM_LOADED

/* form type definitions */

/* ## begin form_types */
#define ft_omega               0       /* undefined (omega)                 */
#define ft_atom                1       /* atoms                             */
#define ft_short               2       /* short integer                     */
#define ft_label               3       /* branch label                      */
#define ft_file                4       /* file pointer                      */
#define ft_opaque              5       /* specifier pointer                 */
#define ft_proc                6       /* procedure                         */
#define ft_process             7       /* process                           */
#define ft_mailbox             8       /* mailbox                           */
#define ft_iter                9       /* iterator                          */
#define ft_real               10       /* real                              */
#define ft_long               11       /* integer                           */
#define ft_string             12       /* string                            */
#define ft_tuple              13       /* tuple                             */
#define ft_object             14       /* object                            */
#define ft_set                15       /* set                               */
#define ft_map                16       /* map                               */
#define ft_void               17       /* used to reset the callback        */
/* ## end form_types */

/*\
 *  \table{form descriptions}
\*/

#ifdef SHARED

char *form_desc[] = {                  /* print string for forms            */
/* ## begin form_desc */
   "omega",                            /* undefined (omega)                 */
   "atom",                             /* atoms                             */
   "integer",                          /* short integer                     */
   "specifier",                        /* specifier pointer                 */
   "label",                            /* branch label                      */
   "file",                             /* file pointer                      */
   "procedure",                        /* procedure                         */
   "process",                          /* process                           */
   "mailbox",                          /* mailbox                           */
   "iterator",                         /* iterator                          */
   "real",                             /* real                              */
   "integer",                          /* integer                           */
   "string",                           /* string                            */
   "tuple",                            /* tuple                             */
   "object",                           /* object                            */
   "set",                              /* set                               */
   "map",                              /* map                               */
/* ## end form_desc */
    NULL};

#else

extern char *form_desc[];              /* print string for forms            */

#endif

#define FORM_LOADED 1
#endif
