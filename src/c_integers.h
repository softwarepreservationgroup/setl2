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
 *  \packagespec{Integer Literal Table}
\*/

#ifndef COMPILER_INTEGERS_LOADED

#define INT_CELL_WIDTH    (sizeof(int32) * 4 - 1)
                                       /* width of integer cell in bits     */
#define MAX_INT_CELL      ((1L << INT_CELL_WIDTH) - 1)
                                       /* maximum value of a cell           */

/* integer table node structure */

struct integer_item {
   struct integer_item *i_next;        /* next cell in list                 */
   struct integer_item *i_prev;        /* previous cell in list             */
   int32 i_value;                      /* integer value                     */
   unsigned i_is_negative;             /* sign field                        */
};

typedef struct integer_item *integer_ptr_type;
                                       /* node pointer                      */

/* clear a integer table item */

#define clear_integer(i) { \
   (i)->i_next = NULL;                 (i)->i_prev = NULL; \
   (i)->i_value = 0;                   (i)->i_is_negative = 0; \
}

/* public function declarations */

void init_integers(void);              /* initialize the integer literal    */
                                       /* table                             */
integer_ptr_type get_integer(SETL_SYSTEM_PROTO_VOID);    
                                       /* allocate a new item               */
void free_compiler_integer(integer_ptr_type);   
                                       /* return an item to the free pool   */
integer_ptr_type char_to_int(SETL_SYSTEM_PROTO char *);  
                                       /* convert string to integer         */

#define COMPILER_INTEGERS_LOADED 1
#endif
