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
 *  \packagespec{String Table}
\*/

#ifndef COMPILER_STRINGS_LOADED

/* name table item structure */

struct string_item {
   char *s_value;                      /* string proper                     */
   int s_length;                       /* string length                     */
};

typedef struct string_item *string_ptr_type;
                                       /* string table type definition      */

/* clear a string table item */

#define clear_string(s) { \
   (s)->s_value = NULL;                (s)->s_length = 0; \
}

/* public function declarations */

void init_strings(SETL_SYSTEM_PROTO_VOID);               
                                       /* initialize string table           */
string_ptr_type get_string(SETL_SYSTEM_PROTO char *, int);
                                       /* get string table item             */
string_ptr_type char_to_string(SETL_SYSTEM_PROTO char *);
                                       /* convert character to string       */

#define COMPILER_STRINGS_LOADED 1
#endif
