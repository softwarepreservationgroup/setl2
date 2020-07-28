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
 *  \packagespec{Real Numbers}
\*/

#ifndef REALS_LOADED

/* real node structure */

struct i_real_item {
   int32 r_use_count;                  /* usage count                       */
   double r_value;                     /* real value                        */
};

typedef struct i_real_item *i_real_ptr_type;
                                       /* real pointer                      */

/* global data */

#ifdef TSAFE
#define REAL_NEXT_FREE plugin_instance->real_next_free 
#else
#define REAL_NEXT_FREE real_next_free 

#ifdef SHARED

i_real_ptr_type real_next_free = NULL;
                                       /* next free header                  */

#else

extern i_real_ptr_type real_next_free;
                                       /* next free header                  */

#endif
#endif

/* allocate and free real nodes */

#ifdef HAVE_MPATROL

#define i_get_real(t) {\
   t = (i_real_ptr_type)malloc(sizeof(struct i_real_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define i_free_real(s) free(s)

#else

#define i_get_real(t) {\
   if (REAL_NEXT_FREE == NULL) i_alloc_reals(SETL_SYSTEM_VOID); \
   t = REAL_NEXT_FREE; \
   REAL_NEXT_FREE = *((i_real_ptr_type *)(REAL_NEXT_FREE)); \
}

#define i_free_real(s) {\
   *((i_real_ptr_type *)(s)) = REAL_NEXT_FREE; \
   REAL_NEXT_FREE = s; \
}

#endif
/* public function declarations */

void init_interp_reals(SETL_SYSTEM_PROTO_VOID);      
                                      /* plant error trap                  */
void i_alloc_reals(SETL_SYSTEM_PROTO_VOID);        
                                      /* allocate a block of header nodes  */
i_real_ptr_type i_copy_real(SETL_SYSTEM_PROTO i_real_ptr_type);
                                       /* copy a real item                  */
double i_real_value(struct specifier_item *);
                                       /* value of real                     */

#define REALS_LOADED 1
#endif


