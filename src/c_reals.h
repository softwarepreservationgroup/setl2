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
 *  \packagespec{Real Literal Table}
\*/

#ifndef COMPILER_REALS_LOADED

/* real table node structure */

struct c_real_item {
   double r_value;                     /* real value                        */
};

typedef struct c_real_item *c_real_ptr_type;
                                       /* node pointer                      */

/* clear a real table item */

#define clear_real(r) { \
   (r)->r_value = 0.0; \
}

/* public function declarations */

void init_compiler_reals(SETL_SYSTEM_PROTO_VOID);     
                                       /* initialize the real literal table */
c_real_ptr_type get_real(SETL_SYSTEM_PROTO_VOID);
                                       /* allocate a new item               */
void free_real(c_real_ptr_type);       /* return an item to the free pool   */
c_real_ptr_type char_to_real(SETL_SYSTEM_PROTO char *, file_pos_type *);
                                       /* convert char to internal real     */

#define COMPILER_REALS_LOADED 1
#endif
