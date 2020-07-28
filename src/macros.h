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
\*/

#ifndef MACROS_LOADED
#define MACROS_LOADED
/* SETL2 system header files */

#ifdef __cplusplus
extern "C" {
#endif

#include "system.h"                    /* SETL2 system constants            */

/* At this point, EXTERNAL has been defined by system.h. 
   Since we are compiling a native package, redefine it to fix the
   win32 brain damaged dllimport. */

#undef EXTERNAL
#ifdef WIN32
#define EXTERNAL __declspec(dllimport) 
#else
#define EXTERNAL extern
#endif

#include "interp.h"                    /* SETL2 interpreter constants       */
#include "abend.h"                     /* abnormal end handler              */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "specs.h"                     /* specifiers                        */
#include "tuples.h"                    /* tuples                            */
#include "sets.h"                      /* sets                              */
#include "assert.h"
#include "builtins.h"                  /* built-in symbols                  */
#include "libman.h"                    /* library manager                   */
#include "unittab.h"                   /* unit table                        */
#include "loadunit.h"                  /* unit loader                       */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_strngs.h"                  /* strings                           */
#include "x_reals.h"                   /* real numbers                      */

#ifndef SETL_OK
#define SETL_OK 0
#endif

#ifndef SETL_ERROR
#define SETL_ERROR 1
#endif

#define SETL_CONCAT(a,b) a##b

#define STRING_CONSTRUCTOR(source)              \
string_h_ptr_type source##_string_hdr;       \
string_c_ptr_type source##_string_cell;      \
char *source##_char_ptr,*source##_char_end;  

#define STRING_CONSTRUCTOR_BEGIN(source) \
   get_string_header(source##_string_hdr); \
   source##_string_hdr->s_use_count = 1; \
   source##_string_hdr->s_hash_code = -1; \
   source##_string_hdr->s_length = 0; \
   source##_string_hdr->s_head = source##_string_hdr->s_tail = NULL; \
   source##_char_ptr = source##_char_end = NULL;

#define STRING_CONSTRUCTOR_ADD(source,char) \
 if (source##_char_ptr == source##_char_end) { \
         get_string_cell(source##_string_cell); \
         if (source##_string_hdr->s_tail != NULL) \
            (source##_string_hdr->s_tail)->s_next = source##_string_cell; \
         source##_string_cell->s_prev = source##_string_hdr->s_tail; \
         source##_string_cell->s_next = NULL; \
         source##_string_hdr->s_tail = source##_string_cell; \
         if (source##_string_hdr->s_head == NULL) \
            source##_string_hdr->s_head = source##_string_cell; \
         source##_char_ptr = source##_string_cell->s_cell_value; \
         source##_char_end = source##_char_ptr + STR_CELL_WIDTH; \
      } \
      *source##_char_ptr++ = char; \
      source##_string_hdr->s_length++; 

#define STRING_HEADER(ca) \
  (ca##_string_hdr)


#define STRING_ITERATOR(source)              \
string_h_ptr_type source##_string_hdr;       \
string_c_ptr_type source##_string_cell;      \
char *source##_char_ptr,*source##_char_end;  \
char *source##_s,*source##_t;                \
int source##_len;

#define STRING_CELL(source)  string_c_ptr_type source;

#define ITERATE_STRING_BEGIN(source,troot)    \
  source##_string_hdr = troot.sp_val.sp_string_ptr;    \
  source##_string_cell = source##_string_hdr->s_head; \
  source##_char_ptr = source##_string_cell->s_cell_value; \
  source##_char_end =  source##_char_ptr + STR_CELL_WIDTH; \
  source##_len = source##_string_hdr->s_length;


#define ITERATE_STRING_CHAR(source) \
  (*(source##_char_ptr))

#define ITERATE_STRING_PTR(source) \
  (source##_char_ptr)

#define ITERATE_STRING_SET_PTR(source) \
  source##_char_ptr=source;

#define ITERATE_STRING_CELL(source) \
  (source##_string_cell)

#define ITERATE_STRING_SET_CELL(source) \
  source##_string_cell=source; \
  source##_char_ptr = source##_string_cell->s_cell_value; \
  source##_char_end = source##_char_ptr + STR_CELL_WIDTH; 

#define ITERATE_STRING_NEXT(source) \
  if (++source##_char_ptr>=source##_char_end) { \
     source##_string_cell = source##_string_cell->s_next;\
     source##_char_ptr = source##_string_cell->s_cell_value; \
     source##_char_end = source##_char_ptr + STR_CELL_WIDTH; \
  }
  

#define STRING_LEN(troot) \
  ((troot.sp_val.sp_string_ptr)->s_length) 


#define STRING_CONVERT(source,dest)           \
   source##_t = dest;                                           \
   for (source##_string_cell = source##_string_hdr->s_head;       \
        source##_string_cell != NULL;                             \
        source##_string_cell = source##_string_cell->s_next) {    \
      for (source##_s = source##_string_cell->s_cell_value;       \
           source##_t < dest + source##_len &&                  \
              source##_s < source##_string_cell->s_cell_value + STR_CELL_WIDTH;\
           *source##_t++ = *source##_s++);                        \
   }                                                              \
   *source##_t = '\0';

#define TUPLE_LEN(troot)              \
 (troot.sp_val.sp_tuple_ptr->t_ntype.t_root.t_length)

#define TUPLE_ITERATOR(source)                \
tuple_h_ptr_type source##_work_hdr;           \
tuple_c_ptr_type source##_cell;               \
int32 source##_number;                        \
int source##_height, source##_index;          \
specifier *source##_element; 

#define ITERATE_TUPLE_BEGIN(source,troot) \
   source##_work_hdr = troot.sp_val.sp_tuple_ptr; \
   source##_number = -1; \
   source##_height = source##_work_hdr->t_ntype.t_root.t_height; \
   source##_index = 0; \
   while (source##_number < troot.sp_val.sp_tuple_ptr->t_ntype.t_root.t_length) { \
      source##_element = NULL; \
      for (;;) { \
	if (!source##_height && source##_index < TUP_HEADER_SIZE) { \
	    if (source##_work_hdr->t_child[source##_index].t_cell == NULL) { \
	       source##_number++; source##_index++; continue; \
	    } \
	    source##_cell = source##_work_hdr->t_child[source##_index].t_cell; \
            source##_element = &(source##_cell->t_spec); \
            source##_number++; source##_index++; \
            break; } \
         if ( source##_index >= TUP_HEADER_SIZE) {  \
            if ( source##_work_hdr == troot.sp_val.sp_tuple_ptr) break; \
            source##_height++; \
            source##_index = \
               source##_work_hdr->t_ntype.t_intern.t_child_index + 1; \
            source##_work_hdr = \
               source##_work_hdr->t_ntype.t_intern.t_parent; \
            continue; \
         }  \
         if (source##_work_hdr->t_child[source##_index].t_header == NULL) { \
            source##_number += 1L << (source##_height * TUP_SHIFT_DIST); \
            source##_index++; \
            continue; \
         } \
         source##_work_hdr = source##_work_hdr->t_child[source##_index].t_header; \
         source##_index = 0; source##_height--;  \
      } \
      if (source##_element == NULL) break;

#define ITERATE_TUPLE_END(source) }

#define TUPLE_CONSTRUCTOR(tc)     \
tuple_h_ptr_type tc##_tuple_root; \
tuple_h_ptr_type tc##_tuple_work_hdr; \
tuple_h_ptr_type tc##_new_tuple_hdr; \
tuple_c_ptr_type tc##_tuple_cell;      \
int tc##_tuple_index, tc##_tuple_height; \
int32 tc##_tuple_length;              \
int32 tc##_i; \
int32 tc##_expansion_trigger;

#define TUPLE_CONSTRUCTOR_BEGIN(con)     \
get_tuple_header(con##_tuple_root); \
con##_tuple_root->t_use_count = 1; \
con##_tuple_root->t_hash_code = 0; \
con##_tuple_root->t_ntype.t_root.t_length = 0; \
con##_tuple_root->t_ntype.t_root.t_height = 0; \
for (con##_i = 0; con##_i < TUP_HEADER_SIZE; \
     con##_tuple_root->t_child[con##_i++].t_cell = NULL); \
con##_tuple_length = 0; \
con##_expansion_trigger = TUP_HEADER_SIZE

#define iTUPLE_ADD_BEGIN(con) \
      if (con##_tuple_length >= con##_expansion_trigger) {\
         con##_tuple_work_hdr = con##_tuple_root;\
         get_tuple_header(con##_tuple_root);\
         con##_tuple_root->t_use_count = 1;\
         con##_tuple_root->t_hash_code =\
            con##_tuple_work_hdr->t_hash_code;\
         con##_tuple_root->t_ntype.t_root.t_length =\
            con##_tuple_work_hdr->t_ntype.t_root.t_length;\
         con##_tuple_root->t_ntype.t_root.t_height =\
            con##_tuple_work_hdr->t_ntype.t_root.t_height + 1;\
         for (con##_i = 1;\
              con##_i < TUP_HEADER_SIZE;\
              con##_tuple_root->t_child[con##_i++].t_header = NULL);\
\
         con##_tuple_root->t_child[0].t_header = con##_tuple_work_hdr;\
\
         con##_tuple_work_hdr->t_ntype.t_intern.t_parent = con##_tuple_root;\
         con##_tuple_work_hdr->t_ntype.t_intern.t_child_index = 0;\
\
         con##_expansion_trigger *= TUP_HEADER_SIZE;\
\
      }\
\
      con##_tuple_root->t_ntype.t_root.t_length++;\
\
      con##_tuple_work_hdr = con##_tuple_root;\
      for (con##_tuple_height = con##_tuple_work_hdr->t_ntype.t_root.t_height;\
           con##_tuple_height;\
           con##_tuple_height--) {\
\
\
         con##_tuple_index = (con##_tuple_length >>\
                              (con##_tuple_height * TUP_SHIFT_DIST)) &\
                           TUP_SHIFT_MASK;\
\
         if (con##_tuple_work_hdr->t_child[con##_tuple_index].t_header == NULL) {\
\
            get_tuple_header(con##_new_tuple_hdr);\
            con##_new_tuple_hdr->t_ntype.t_intern.t_parent = con##_tuple_work_hdr;\
            con##_new_tuple_hdr->t_ntype.t_intern.t_child_index = con##_tuple_index;\
            for (con##_i = 0;\
                 con##_i < TUP_HEADER_SIZE;\
                 con##_new_tuple_hdr->t_child[con##_i++].t_cell = NULL);\
            con##_tuple_work_hdr->t_child[con##_tuple_index].t_header =\
                  con##_new_tuple_hdr;\
            con##_tuple_work_hdr = con##_new_tuple_hdr;\
\
         } else {\
\
            con##_tuple_work_hdr =\
               con##_tuple_work_hdr->t_child[con##_tuple_index].t_header;\
\
         }\
      }\
\
      con##_tuple_index = con##_tuple_length & TUP_SHIFT_MASK;\
      get_tuple_cell(con##_tuple_cell)

#define iTUPLE_ADD_END(con) \
      spec_hash_code(con##_tuple_cell->t_hash_code,&(con##_tuple_cell->t_spec));\
      con##_tuple_root->t_hash_code ^= con##_tuple_cell->t_hash_code;\
      con##_tuple_work_hdr->t_child[con##_tuple_index].t_cell = con##_tuple_cell;\
\
      con##_tuple_length++

#define TUPLE_CONSTRUCTOR_END(con) \
   con##_tuple_root->t_ntype.t_root.t_length = con##_tuple_length

#define TUPLE_ADD_CELL(ca,right) \
  iTUPLE_ADD_BEGIN(ca); \
  ca##_tuple_cell->t_spec.sp_form = (right)->sp_form; \
  ca##_tuple_cell->t_spec.sp_val.sp_biggest = \
               (right)->sp_val.sp_biggest; \
  iTUPLE_ADD_END(ca);

#define TUPLE_HEADER(ca) \
  ca##_tuple_root

/***********************************************************************/

#define SET_ITERATOR(source)                \
set_h_ptr_type source##_work_hdr;           \
set_c_ptr_type source##_cell;               \
int32 source##_number;                        \
int source##_height, source##_index;          \
specifier *source##_element; 

#define ITERATE_SET_BEGIN(source,troot) \
   source##_work_hdr = troot.sp_val.sp_set_ptr; \
   source##_number = 0; \
   source##_height = troot.sp_val.sp_set_ptr->s_ntype.s_root.s_height; \
   source##_index = 0; \
   source##_cell = NULL; \
   while (source##_number < troot.sp_val.sp_set_ptr->s_ntype.s_root.s_cardinality) { \
   for (;;) { \
      if (source##_cell != NULL) { \
         source##_element=&(source##_cell->s_spec);\
         source##_cell = source##_cell->s_next; \
         source##_number++; \
	 break; \
      } \
      if (!source##_height && source##_index < SET_HASH_SIZE) { \
         source##_cell = source##_work_hdr->s_child[source##_index].s_cell; \
         source##_index++; \
         continue; \
      } \
      if (source##_index >= SET_HASH_SIZE) { \
            if (source##_work_hdr == troot.sp_val.sp_set_ptr) \
               break; \
            source##_height++; \
            source##_index = \
               source##_work_hdr->s_ntype.s_intern.s_child_index + 1; \
            source##_work_hdr = \
               source##_work_hdr->s_ntype.s_intern.s_parent; \
            continue; \
         } \
      if (source##_work_hdr->s_child[source##_index].s_header == NULL) { \
         source##_index++; \
         continue; \
      } \
      source##_work_hdr = source##_work_hdr->s_child[source##_index].s_header; \
      source##_index = 0; \
      source##_height--; \
   } 

#define ITERATE_SET_END(source) }

#define SET_CONSTRUCTOR(tc)     \
set_h_ptr_type tc##_set_root; \
set_h_ptr_type tc##_set_work_hdr; \
set_h_ptr_type tc##_new_set_hdr; \
set_c_ptr_type tc##_set_cell,*tc##_set_cell_tail, tc##_new_set_cell; \
int tc##_is_equal,tc##_set_index, tc##_set_height, tc##_target_index;\
int32 tc##_set_length;              \
int32 tc##_i; \
int32 tc##_work_hash_code,tc##_source_hash_code; \
int32 tc##_expansion_trigger;

#define SET_CONSTRUCTOR_BEGIN(con)     \
get_set_header(con##_set_root); \
con##_set_root->s_use_count = 1; \
con##_set_root->s_hash_code = 0; \
con##_set_root->s_ntype.s_root.s_cardinality = 0; \
con##_set_root->s_ntype.s_root.s_height = 0; \
for (con##_i = 0; con##_i < SET_HASH_SIZE; \
     con##_set_root->s_child[con##_i++].s_cell = NULL); \
con##_set_length = 0; \
con##_set_height = con##_set_root->s_ntype.s_root.s_height; \
con##_expansion_trigger = SET_HASH_SIZE



#define SET_ADD_CELL(con,right) \
	 con##_set_work_hdr = con##_set_root; \
         spec_hash_code(con##_work_hash_code,right); \
         con##_source_hash_code = con##_work_hash_code; \
 \
         con##_set_height = con##_set_root->s_ntype.s_root.s_height; \
         while (con##_set_height--) { \
 \
            /* extract the element's index at this level */ \
 \
            con##_target_index = con##_work_hash_code & SET_HASH_MASK; \
            con##_work_hash_code = con##_work_hash_code >> SET_SHIFT_DIST; \
 \
            /* if we're missing a header record, insert it */ \
 \
            if (con##_set_work_hdr->s_child[con##_target_index].s_header == NULL) { \
 \
               get_set_header(con##_new_set_hdr); \
               con##_new_set_hdr->s_ntype.s_intern.s_parent = con##_set_work_hdr; \
               con##_new_set_hdr->s_ntype.s_intern.s_child_index = con##_target_index; \
               for (con##_i = 0; \
                    con##_i < SET_HASH_SIZE; \
                    con##_new_set_hdr->s_child[con##_i++].s_cell = NULL); \
               con##_set_work_hdr->s_child[con##_target_index].s_header = con##_new_set_hdr; \
               con##_set_work_hdr = con##_new_set_hdr; \
 \
            } \
            else { \
 \
               con##_set_work_hdr = \
                  con##_set_work_hdr->s_child[con##_target_index].s_header; \
 \
            } \
         } \
 \
         /* \
          *  At this point, set_work_hdr points to the lowest level header \
          *  record.  The next problem is to determine if the element is \
          *  already in the set.  We compare the element with the clash \
          *  list. \
          */ \
 \
         con##_target_index = con##_work_hash_code & SET_HASH_MASK; \
         con##_set_cell_tail = &(con##_set_work_hdr->s_child[con##_target_index].s_cell); \
         for (con##_set_cell = *con##_set_cell_tail; \
              con##_set_cell != NULL && \
                 con##_set_cell->s_hash_code < con##_source_hash_code; \
              con##_set_cell = con##_set_cell->s_next) { \
 \
            con##_set_cell_tail = &(con##_set_cell->s_next); \
 \
         } \
 \
         /* check for a duplicate element */ \
 \
         con##_is_equal = NO; \
         while (con##_set_cell != NULL && \
                con##_set_cell->s_hash_code == con##_source_hash_code) { \
 \
            spec_equal(con##_is_equal,&(con##_set_cell->s_spec),right); \
 \
            if (con##_is_equal) \
               break; \
 \
            con##_set_cell_tail = &(con##_set_cell->s_next); \
            con##_set_cell = con##_set_cell->s_next; \
 \
         } \
 \
         /* if we reach this point we didn't find the element */ \
 \
         if (!con##_is_equal) { \
	    mark_specifier(right); \
            get_set_cell(con##_new_set_cell); \
            con##_new_set_cell->s_spec.sp_form = (right)->sp_form; \
            con##_new_set_cell->s_spec.sp_val.sp_biggest = \
               (right)->sp_val.sp_biggest; \
\
            con##_new_set_cell->s_hash_code = con##_source_hash_code; \
            con##_new_set_cell->s_next = *con##_set_cell_tail; \
            *con##_set_cell_tail = con##_new_set_cell; \
            con##_set_root->s_ntype.s_root.s_cardinality++; \
            con##_set_root->s_hash_code ^= con##_source_hash_code; \
            /* we may have to expand the size of the header */ \
 \
            con##_expansion_trigger = \
                 (1 << ((con##_set_root->s_ntype.s_root.s_height + 1) \
                  * SET_SHIFT_DIST)) * SET_CLASH_SIZE; \
 \
            /* expand the set header if necessary */ \
 \
            if (con##_set_root->s_ntype.s_root.s_cardinality > con##_expansion_trigger) { \
 \
               con##_set_root = set_expand_header(SETL_SYSTEM con##_set_root); \
 \
            } \
         }

#define SET_CONSTRUCTOR_END(con) 

/*
*/
#define SET_LEN(troot)              \
 (troot.sp_val.sp_set_ptr->s_ntype.s_root.s_cardinality)

#define SET_HEADER(ca) \
  ca##_set_root

#ifdef __cplusplus
}
#endif

#endif /* MACROS_LOADED */
