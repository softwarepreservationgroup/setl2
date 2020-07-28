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
 *  \packagespec{Procedure Table}
\*/

#ifndef PROCS_LOADED

/* procedure types */

#define BUILTIN_PROC 0                 /* built-in procedure                */
#define USERDEF_PROC 1                 /* user-defined procedure            */
#define NATIVE_PROC  2                 /* native user-defined procedure     */

/* procedure table node structure */

struct proc_item {
   int32 p_use_count;                  /* usage count                       */
   struct proc_item *p_signature;      /* hash code                         */
   int p_type;                         /* procedure type                    */
   void (*p_func_ptr)(SETL_SYSTEM_PROTO int, struct specifier_item *,
                           struct specifier_item *);
                                       /* built-in procedure                */
   struct unittab_item *p_unittab_ptr;
                                       /* unit owning procedure             */
   int32 p_offset;                     /* offset within unit                */
   struct specifier_item *p_spec_ptr;  /* procedure data                    */
   int32 p_spec_count;                 /* number of specifiers in procedure */
   int32 p_formal_count;               /* number of formal parameters       */
   struct object_h_item *p_self_ptr;   /* current instance                  */
   struct proc_item *p_parent;         /* procedure's parent                */
   struct proc_item *p_copy;           /* copy of procedure                 */
   unsigned p_var_args : 1;            /* YES if procedure accepts a        */
                                       /* variable number of arguments      */
   struct specifier_item *p_save_specs;
                                       /* saved local data                  */
   int p_active_use_count;             /* active procedures using proc      */
   int p_is_const;                     /* YES if constant procedure         */
   int p_current_saved;                /* YES if already saved during       */
                                       /* swap                              */
};

typedef struct proc_item *proc_ptr_type;
                                       /* procedure node pointer            */

/* global data */

#ifdef TSAFE
#define PROC_NEXT_FREE plugin_instance->proc_next_free 
#else
#define PROC_NEXT_FREE proc_next_free 
#ifdef SHARED

proc_ptr_type proc_next_free = NULL;
                                       /* next free procedure               */

#else

extern proc_ptr_type proc_next_free;
                                       /* next free procedure               */

#endif
#endif

/* allocate and free procedure nodes */

#define get_proc(t) {\
   if (PROC_NEXT_FREE == NULL) alloc_procs(SETL_SYSTEM_VOID); \
   t = PROC_NEXT_FREE; \
   PROC_NEXT_FREE = *((proc_ptr_type *)(PROC_NEXT_FREE)); \
}

#define free_proc(s) {\
   *((proc_ptr_type *)(s)) = PROC_NEXT_FREE; \
   PROC_NEXT_FREE = s; \
}

/* public function declarations */

void alloc_procs(SETL_SYSTEM_PROTO_VOID);               
                                       /* allocate a block of procedure     */
                                       /* nodes                             */
void free_procedure(SETL_SYSTEM_PROTO proc_ptr_type);  
                                       /* recursively free a procedure      */

#define PROCS_LOADED 1
#endif


