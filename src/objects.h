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
 *  \packagespec{Objects}
\*/

#ifndef OBJECTS_LOADED

/* performance tuning constants */

#define OBJ_HEADER_SIZE    4           /* object header size (table)        */
#define OBJ_SHIFT_DIST     2           /* log (base 2) of OBJ_HEADER_SIZE   */
#define OBJ_SHIFT_MASK  0x03           /* pick out one level of header tree */

/* object header node structure */

struct object_h_item {
   int32 o_use_count;                  /* usage count                       */
   int32 o_hash_code;                  /* hash code                         */
   struct process_item *o_process_ptr; /* associated process record         */
   union {
      struct {                         /* root header info                  */
         struct unittab_item *o_class; /* class of object                   */
      } o_root;
      struct {                         /* internal node info                */
         struct object_h_item *o_parent;
                                       /* parent in header tree             */
         int o_child_index;            /* index in parent's table           */
      } o_intern;
   } o_ntype;
   union {
      struct object_c_item *o_cell;    /* cell child pointer                */
      struct object_h_item *o_header;  /* internal header node pointer      */
   } o_child[OBJ_HEADER_SIZE];
};

typedef struct object_h_item *object_h_ptr_type;
                                       /* header node pointer               */

/* object cell node structure */

struct object_c_item {
   int32 o_hash_code;                  /* full hash code of element         */
   struct specifier_item o_spec;       /* object element specifier          */
};

typedef struct object_c_item *object_c_ptr_type;
                                       /* cell node pointer                 */

/* we have to keep a stack of 'self's with each class */

struct self_stack_item {
   struct object_h_item *ss_object;    /* self root structure               */
   struct self_stack_item *ss_next;    /* next active self                  */
};

typedef struct self_stack_item *self_stack_ptr_type;
                                       /* pointer to self record            */

/* global data */

#ifdef TSAFE
#define OBJECT_H_NEXT_FREE plugin_instance->object_h_next_free 
#define OBJECT_C_NEXT_FREE plugin_instance->object_c_next_free 
#define SELF_STACK_NEXT_FREE plugin_instance->self_stack_next_free
#else
#define OBJECT_H_NEXT_FREE object_h_next_free 
#define OBJECT_C_NEXT_FREE object_c_next_free 
#define SELF_STACK_NEXT_FREE self_stack_next_free

#ifdef SHARED

object_h_ptr_type object_h_next_free = NULL;
                                       /* next free header                  */
object_c_ptr_type object_c_next_free = NULL;
                                       /* next free cell                    */
self_stack_ptr_type self_stack_next_free = NULL;
                                       /* next free self item               */

#else

extern object_h_ptr_type object_h_next_free;
                                       /* next free header                  */
extern object_c_ptr_type object_c_next_free;
                                       /* next free cell                    */
extern self_stack_ptr_type self_stack_next_free;
                                       /* next free self item               */

#endif
#endif

/* allocate and free header nodes */

#ifdef HAVE_MPATROL

#define get_object_header(t) {\
   t = (object_h_ptr_type)malloc(sizeof(struct object_h_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_object_header(s) free(s)

#define get_object_cell(t) {\
   t = (object_c_ptr_type)malloc(sizeof(struct object_c_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_object_cell(s) free(s)

#else

#define get_object_header(t) {\
   if (OBJECT_H_NEXT_FREE == NULL) alloc_object_headers(SETL_SYSTEM_VOID); \
   t = OBJECT_H_NEXT_FREE; \
   OBJECT_H_NEXT_FREE = *((object_h_ptr_type *)(OBJECT_H_NEXT_FREE)); \
}

#define free_object_header(s) {\
   *((object_h_ptr_type *)(s)) = OBJECT_H_NEXT_FREE; \
   OBJECT_H_NEXT_FREE = s; \
}

/* allocate and free cell nodes */

#define get_object_cell(t) {\
   if (OBJECT_C_NEXT_FREE == NULL) alloc_object_cells(SETL_SYSTEM_VOID); \
   t = OBJECT_C_NEXT_FREE; \
   OBJECT_C_NEXT_FREE = *((object_c_ptr_type *)(OBJECT_C_NEXT_FREE)); \
}

#define free_object_cell(s) {\
   *((object_c_ptr_type *)(s)) = OBJECT_C_NEXT_FREE; \
   OBJECT_C_NEXT_FREE = s; \
}
#endif

/* allocate and free header nodes */

#define get_self_stack(t) {\
   if (SELF_STACK_NEXT_FREE == NULL) alloc_self_stack(SETL_SYSTEM_VOID); \
   t = SELF_STACK_NEXT_FREE; \
   SELF_STACK_NEXT_FREE = *((self_stack_ptr_type *)(SELF_STACK_NEXT_FREE)); \
}

#define free_self_stack(s) {\
   *((self_stack_ptr_type *)(s)) = SELF_STACK_NEXT_FREE; \
   SELF_STACK_NEXT_FREE = s; \
}

/* public function declarations */

void alloc_object_headers(SETL_SYSTEM_PROTO_VOID);       
                                       /* allocate a block of header nodes  */
void alloc_object_cells(SETL_SYSTEM_PROTO_VOID);         
                                       /* allocate a block of cell nodes    */
object_h_ptr_type copy_object(SETL_SYSTEM_PROTO object_h_ptr_type);
                                       /* copy a object structure           */
void free_object(SETL_SYSTEM_PROTO object_h_ptr_type);   
                                       /* free an object structure          */
void alloc_self_stack(SETL_SYSTEM_PROTO_VOID);           
                                       /* allocate a block of self stack    */
                                       /* items                             */
#define OBJECTS_LOADED 1
#endif


