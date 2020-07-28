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
 *  \package{Specifiers}
 *
 *  \texify{specs.h}
 *
 *  \packagebody{Specifiers}
\*/

/* standard C header files */

#if WATCOM
#include <dos.h>                       /* DOS use macros                    */
#endif

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "mcode.h"                     /* method codes                      */
#include "unittab.h"                   /* unit table                        */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_reals.h"                   /* real numbers                      */
#include "x_strngs.h"                  /* strings                           */
#include "sets.h"                      /* sets                              */
#include "maps.h"                      /* maps                              */
#include "tuples.h"                    /* tuples                            */
#include "procs.h"                     /* procedures                        */
#include "objects.h"                   /* objects                           */
#include "process.h"                   /* objects                           */
#include "mailbox.h"                   /* objects                           */
#include "x_files.h"                   /* files                             */
#include "iters.h"                     /* iterators                         */

/*\
 *  \function{register\_type()}
 *
\*/

int32 register_type(
   SETL_SYSTEM_PROTO
   char *t,
   void *destr)
{
int k,f;

   f=NUM_REG_TYPES;
   for (k=0;k<NUM_REG_TYPES;k++) {
        if (REG_TYPES[k].name==NULL) {
            f=k;
            k=NUM_REG_TYPES;
            break;
        }
        if (strcmp(REG_TYPES[k].name,t)==0) return k+1;
   }
 
   if (f==NUM_REG_TYPES) {
   int j;

      /* Allocate more space */
      k=NUM_REG_TYPES *2; 
      if (k==0) {
			k=1;
			REG_TYPES = (setl_destructor *)malloc(sizeof(setl_destructor));
	  } else {
        REG_TYPES = (setl_destructor *)realloc(REG_TYPES,sizeof(setl_destructor)*k);
      }
	  if (REG_TYPES==NULL) return 0;

      for (j=NUM_REG_TYPES;j<k;j++) {
        REG_TYPES[j].name=NULL;
  }

      NUM_REG_TYPES = k;

   }

#ifdef macintosh
   REG_TYPES[f].name=malloc(strlen(t)+1);
   strcpy(REG_TYPES[f].name, t);
#else
   REG_TYPES[f].name=strdup(t);
#endif
   REG_TYPES[f].function = destr;

   return f+1;

}

/*\
 *  \function{get\_specifiers()}
 *
 *  This function allocates an array of specifiers, and initializes them
 *  to omega.
\*/

specifier *get_specifiers(
   SETL_SYSTEM_PROTO
   int32 count)                        /* number of specifiers to allocate  */

{
specifier *return_ptr;                 /* returned array                    */
specifier *s;                          /* temporary looping variable        */

   /* get at least one specifier */

   if (!count)
      count = 1;

   /* allocate the memory */

   return_ptr = (specifier *)malloc((size_t)(count * sizeof(specifier)));
   if (return_ptr == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* initialize them to omega */

   for (s = return_ptr; s < return_ptr + count; s++)
      s->sp_form = ft_omega;

   return return_ptr;

}

/*\
 *  \function{free\_specifier()}
 *
 *  This function frees the memory used by a long data item.  In most
 *  cases we have to loop over cells and headers freeing each.
\*/

void free_specifier(
   SETL_SYSTEM_PROTO
   specifier *spec)                    /* specifier to be released          */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
integer_h_ptr_type integer_hdr;        /* long integer root                 */
integer_c_ptr_type integer_cell_1, integer_cell_2;
                                       /* integer cell pointers             */
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell_1, string_cell_2;
                                       /* string cell pointers              */
tuple_h_ptr_type tuple_root, tuple_work_hdr, tuple_save_hdr;
                                       /* tuple work headers                */
set_h_ptr_type set_root, set_work_hdr, set_save_hdr;
                                       /* set work headers                  */
set_c_ptr_type set_cell_1, set_cell_2; /* set cell pointers                 */
map_h_ptr_type map_root, map_work_hdr, map_save_hdr;
                                       /* map work headers                  */
map_c_ptr_type map_cell_1, map_cell_2; /* map cell pointers                 */
object_h_ptr_type object_root, object_work_hdr, object_save_hdr;
                                       /* object work headers               */
unittab_ptr_type class_ptr;            /* object class                      */
int height;                            /* working height of header tree     */
int index;                             /* index within header hash table    */

   switch (spec->sp_form) {

/*\
 *  \case{simple types}
 *
 *  For several types we need just free a node using a simple macro.
\*/

case ft_proc :
  
   free_procedure(SETL_SYSTEM spec->sp_val.sp_proc_ptr);

   return;

case ft_real :

   i_free_real(spec->sp_val.sp_real_ptr);

   return;

/*\
 *  \case{iterators}
 *
 *  We free the object we are iterating over, then the iterator node.
\*/

case ft_iter :

{

   iter_ptr = spec->sp_val.sp_iter_ptr;

   /* release any specifiers locked by the iterator */

   switch (iter_ptr->it_type) {

      case it_set :

         unmark_specifier(&(iter_ptr->it_itype.it_setiter.it_spec));

         break;

      case it_map :

         unmark_specifier(&(iter_ptr->it_itype.it_mapiter.it_spec));

         break;

      case it_tuple :

         unmark_specifier(&(iter_ptr->it_itype.it_tupiter.it_spec));

         break;

      case it_string :

         unmark_specifier(&(iter_ptr->it_itype.it_striter.it_spec));

         break;

      case it_domain :

         unmark_specifier(&(iter_ptr->it_itype.it_mapiter.it_spec));

         break;

      case it_pow :

         unmark_specifier(&(iter_ptr->it_itype.it_powiter.it_spec));
         free(iter_ptr->it_itype.it_powiter.it_se_array);

         break;

      case it_npow :

         unmark_specifier(&(iter_ptr->it_itype.it_powiter.it_spec));
         free(iter_ptr->it_itype.it_powiter.it_se_array);

         break;

      case it_map_pair :

         unmark_specifier(&(iter_ptr->it_itype.it_mapiter.it_spec));

         break;

      case it_tuple_pair :

         unmark_specifier(&(iter_ptr->it_itype.it_tupiter.it_spec));

         break;

      case it_string_pair :

         unmark_specifier(&(iter_ptr->it_itype.it_striter.it_spec));

         break;

      case it_map_multi :

         unmark_specifier(&(iter_ptr->it_itype.it_mapiter.it_spec));

         break;

      case it_object :

         unmark_specifier(&(iter_ptr->it_itype.it_objiter.it_spec));

         break;

      case it_object_pair :

         unmark_specifier(&(iter_ptr->it_itype.it_objiter.it_spec));

         break;

   }

   /* free the iterator node itself */

   free_iterator(iter_ptr);

   return;

}

/*\
 *  \case{long integers}
 *
 *  We free the linked list of cells and the header node.
\*/

case ft_long :

{

   integer_hdr = spec->sp_val.sp_long_ptr;

   integer_cell_1 = integer_hdr->i_head;
   while (integer_cell_1 != NULL) {

      integer_cell_2 = integer_cell_1;
      integer_cell_1 = integer_cell_1->i_next;
      free_integer_cell(integer_cell_2);

   }

   free_integer_header(integer_hdr);

   return;

}

/*\
 *  \case{mailboxes}
 *
 *  We free the linked list of cells and the header node.
\*/

case ft_mailbox :

{      
mailbox_h_ptr_type header;             /* mailbox header                    */
mailbox_c_ptr_type t1,t2;              /* temporary looping variables       */

   header = spec->sp_val.sp_mailbox_ptr;
   t1 = header->mb_head;

   while (t1 != NULL) {

      t2 = t1;
      t1 = t1->mb_next;
      unmark_specifier(&(t2->mb_spec));
      free_mailbox_cell(t2);

   }

   free_mailbox_header(header);

   return;

}

/*\
 *  \case{strings}
 *
 *  We free the linked list of cells and the header node.
\*/

case ft_string :

{

   string_hdr = spec->sp_val.sp_string_ptr;

   string_cell_1 = string_hdr->s_head;
   while (string_cell_1 != NULL) {

      string_cell_2 = string_cell_1;
      string_cell_1 = string_cell_1->s_next;
      free_string_cell(string_cell_2);

   }

   free_string_header(string_hdr);

   return;

}

/*\
 *  \case{tuples}
 *
 *  We free cells and headers, using a normal algorithm to loop over the
 *  structure.
\*/

case ft_tuple :

{

   tuple_root = spec->sp_val.sp_tuple_ptr;

   /* we start iterating from the root, at the left of the header table */

   height = tuple_root->t_ntype.t_root.t_height;
   tuple_work_hdr = tuple_root;
   index = 0;

   /* delete nodes until we finish the tree */

   for (;;) {

      /* if we're at a leaf, delete all the tuple elements */

      if (!height) {

         for (index = 0; index < TUP_HEADER_SIZE; index++) {

            if (tuple_work_hdr->t_child[index].t_cell != NULL) {

               unmark_specifier(
                  &((tuple_work_hdr->t_child[index].t_cell)->t_spec));
               free_tuple_cell(tuple_work_hdr->t_child[index].t_cell);

            }
         }
      }

      /* if we've finished an internal node, move up */

      if (index >= TUP_HEADER_SIZE) {

         /* when we get back to the root we're finished */

         if (tuple_work_hdr == tuple_root) {
            break;
         }

         height++;
         index = tuple_work_hdr->t_ntype.t_intern.t_child_index + 1;
         tuple_save_hdr = tuple_work_hdr;
         tuple_work_hdr = tuple_work_hdr->t_ntype.t_intern.t_parent;
         free_tuple_header(tuple_save_hdr);

         continue;

      }

      /* if we can't move down, continue */

      if (tuple_work_hdr->t_child[index].t_header == NULL) {

         index++;
         continue;

      }

      /* we can move down, so do so */

      tuple_work_hdr = tuple_work_hdr->t_child[index].t_header;
      index = 0;
      height--;

   }

   free_tuple_header(tuple_root);

   return;

}

/*\
 *  \case{sets}
 *
 *  We free cells and headers, using a normal algorithm to loop over the
 *  structure.
\*/

case ft_set :

{

   set_root = spec->sp_val.sp_set_ptr;

   /* we start iterating from the root, at the left of the hash table */

   height = set_root->s_ntype.s_root.s_height;
   set_work_hdr = set_root;
   index = 0;

   /* delete nodes until we finish the tree */

   for (;;) {

      /* if we're at a leaf, delete all the set elements */

      if (!height) {

         for (index = 0; index < SET_HASH_SIZE; index++) {

            set_cell_1 = set_work_hdr->s_child[index].s_cell;
            while (set_cell_1 != NULL) {

               set_cell_2 = set_cell_1;
               set_cell_1 = set_cell_1->s_next;
               unmark_specifier(&(set_cell_2->s_spec));
               free_set_cell(set_cell_2);

            }
         }
      }

      /* if we've finished a header node, move up */

      if (index >= SET_HASH_SIZE) {

         /* when we return to the root we're done */

         if (set_work_hdr == set_root)
            break;

         height++;
         index = set_work_hdr->s_ntype.s_intern.s_child_index + 1;
         set_save_hdr = set_work_hdr;
         set_work_hdr = set_work_hdr->s_ntype.s_intern.s_parent;
         free_set_header(set_save_hdr);
         continue;

      }

      /* if we can't move down, continue */

      if (set_work_hdr->s_child[index].s_header == NULL) {

         index++;
         continue;

      }

      /* we can move down, so do so */

      set_work_hdr = set_work_hdr->s_child[index].s_header;
      index = 0;
      height--;

   }

   free_set_header(set_root);

   return;

}

/*\
 *  \case{maps}
 *
 *  We free cells and headers, using a normal algorithm to loop over the
 *  structure.
\*/

case ft_map :

{

   map_root = spec->sp_val.sp_map_ptr;

   /* we start iterating from the root, at the left of the hash table */

   height = map_root->m_ntype.m_root.m_height;
   map_work_hdr = map_root;
   index = 0;

   /* delete nodes until we finish the tree */

   for (;;) {

      /* if we're at a leaf, delete all the map elements */

      if (!height) {

         for (index = 0; index < MAP_HASH_SIZE; index++) {

            map_cell_1 = map_work_hdr->m_child[index].m_cell;
            while (map_cell_1 != NULL) {

               map_cell_2 = map_cell_1;
               map_cell_1 = map_cell_1->m_next;
               unmark_specifier(&(map_cell_2->m_domain_spec));
               unmark_specifier(&(map_cell_2->m_range_spec));
               free_map_cell(map_cell_2);

            }
         }
      }

      /* if we've finished a header node, move up */

      if (index >= MAP_HASH_SIZE) {

         /* when we return to the root we're done */

         if (map_work_hdr == map_root)
            break;

         height++;
         index = map_work_hdr->m_ntype.m_intern.m_child_index + 1;
         map_save_hdr = map_work_hdr;
         map_work_hdr = map_work_hdr->m_ntype.m_intern.m_parent;
         free_map_header(map_save_hdr);
         continue;

      }

      /* if we can't move down, continue */

      if (map_work_hdr->m_child[index].m_header == NULL) {

         index++;
         continue;

      }

      /* we can move down, so do so */

      map_work_hdr = map_work_hdr->m_child[index].m_header;
      index = 0;
      height--;

   }

   free_map_header(map_root);

   return;

}

/*\
 *  \case{objects}
 *
 *  We free cells and headers, using a normal algorithm to loop over the
 *  structure.
\*/

case ft_process :

{
process_ptr_type process_ptr;

   object_root = spec->sp_val.sp_object_ptr;

   process_ptr = object_root->o_process_ptr;
   process_ptr->pc_prev->pc_next = process_ptr->pc_next;
   process_ptr->pc_next->pc_prev = process_ptr->pc_prev;
   free(process_ptr->pc_pstack);
   free(process_ptr->pc_cstack);

}

/* NOTICE:  Deliberate fall through from previous case */

case ft_object :

{

   object_root = spec->sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* we start iterating from the root, at the left of the header table */

   height = class_ptr->ut_obj_height;
   object_work_hdr = object_root;
   index = 0;

   /* delete nodes until we finish the tree */

   for (;;) {

      /* if we're at a leaf, delete all the object elements */

      if (!height) {

         for (index = 0; index < OBJ_HEADER_SIZE; index++) {

            if (object_work_hdr->o_child[index].o_cell != NULL) {

               unmark_specifier(
                  &((object_work_hdr->o_child[index].o_cell)->o_spec));
               free_object_cell(object_work_hdr->o_child[index].o_cell);

            }
         }
      }

      /* if we've finished an internal node, move up */

      if (index >= OBJ_HEADER_SIZE) {

         /* when we get back to the root we're finished */

         if (object_work_hdr == object_root)
            break;

         height++;
         index = object_work_hdr->o_ntype.o_intern.o_child_index + 1;
         object_save_hdr = object_work_hdr;
         object_work_hdr = object_work_hdr->o_ntype.o_intern.o_parent;
         free_object_header(object_save_hdr);

         continue;

      }

      /* if we can't move down, continue */

      if (object_work_hdr->o_child[index].o_header == NULL) {

         index++;
         continue;

      }

      /* we can move down, so do so */

      object_work_hdr = object_work_hdr->o_child[index].o_header;
      index = 0;
      height--;

   }

   free_object_header(object_root);

   return;

}

case ft_opaque :

{
int typ;
void *dest;
   
   /* Get the native type */

   typ = ((spec->sp_val.sp_opaque_ptr->type) & 65535) -1;

   if ((typ<0)||(typ>=NUM_REG_TYPES))
        return;

   dest = REG_TYPES[typ].function;
   if (dest)
   	(*(void(*)(specifier *))dest)((specifier *)spec->sp_val.sp_opaque_ptr);

}
   /* return to normal indentation */

   }
}

/*\
 *  \function{spec\_equal()}
 *
 *  This macro makes a rough cut at an equality test.  If the two
 *  specifiers are short and equal we can determine equality without a
 *  procedure call.  If they are long but of different forms we also
 *  avoid a procedure call.  Otherwise we resort to a more robust
 *  function.
\*/

int spec_equal_mac(
   SETL_SYSTEM_PROTO
   specifier *l,
   specifier *r)

{

   if ((l)->sp_form == ft_omega && (r)->sp_form == ft_omega)
      return 1;
   else if ((l)->sp_form == (r)->sp_form &&
            (l)->sp_val.sp_biggest == (r)->sp_val.sp_biggest) 
      return 1;
   else if ((l)->sp_form < ft_real || (r)->sp_form < ft_real)
      return 0;
   else if (((l)->sp_form < ft_set || (r)->sp_form < ft_set) && 
            (l)->sp_form != (r)->sp_form)
      return 0;
   else return spec_equal_test(SETL_SYSTEM l,r);

}

/*\
 *  \function{spec\_equal\_test()}
 *
 *  This is a second-level function to determine whether two specifiers
 *  have the same value.  We depend on the macro \verb"spec_equal" to
 *  handle the easy cases.
\*/

int spec_equal_test(
   SETL_SYSTEM_PROTO
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
integer_h_ptr_type left_integer_hdr, right_integer_hdr;
                                       /* integer root pointers             */
integer_c_ptr_type left_integer_cell, right_integer_cell;
                                       /* integer cell pointers             */
string_h_ptr_type left_string_hdr, right_string_hdr;
                                       /* string root pointers              */
string_c_ptr_type left_string_cell, right_string_cell;
                                       /* string cell pointers              */
char *left_char_ptr, *right_char_ptr;  /* string character pointers         */
int32 string_length;                   /* work string length                */
int32 string_cell_length;              /* cell string length                */
tuple_h_ptr_type left_tuple_root, left_tuple_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type left_tuple_cell;      /* current cell pointer              */
int32 left_number;                     /* current cell number               */
int left_height, left_index;           /* current height and index          */
specifier *left_element;               /* set element                       */
int left_element_needed;               /* YES if we should extract an       */
                                       /* element                           */
tuple_h_ptr_type right_tuple_root, right_tuple_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type right_tuple_cell;     /* current cell pointer              */
int32 right_number;                    /* current cell number               */
int right_height, right_index;         /* current height and index          */
specifier *right_element;              /* set element                       */
int right_element_needed;              /* YES if we should extract an       */
                                       /* element                           */
set_h_ptr_type left_set_root, left_set_work_hdr, new_set_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type left_set_list;          /* clash list                        */
set_c_ptr_type left_set_cell;          /* cell pointer                      */
set_h_ptr_type right_set_root, right_set_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type right_set_list;         /* clash list                        */
set_c_ptr_type right_set_cell, right_set_work_cell;
                                       /* cell pointers                     */
map_h_ptr_type left_map_root, left_map_work_hdr, new_map_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type left_map_list;          /* clash list                        */
map_c_ptr_type left_map_cell;          /* cell pointer                      */
map_h_ptr_type right_map_root, right_map_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type right_map_list;         /* clash list                        */
map_c_ptr_type right_map_cell, right_map_work_cell, save_map_work_cell;
                                       /* cell pointers                     */
object_h_ptr_type left_object_root, left_object_work_hdr;
object_h_ptr_type right_object_root, right_object_work_hdr;
                                       /* root and internal node pointers   */
unittab_ptr_type class_ptr;            /* class of objects                  */
int is_equal;                          /* YES if two specifiers have the    */
                                       /* same value                        */
specifier spare;                       /* spare specifier                   */
int i;                                 /* temporary looping variable        */

   /*
    *  The short cases and those for which we demand structural
    *  equivalence are already handled.  We also know that the form codes
    *  are equal unless they are maps and sets.
    */

   spare.sp_form = ft_omega;


   switch (left->sp_form) {


/*\
 *  \case{reals}
 *
 *  Reals are equal if their respective values are equal.
\*/

case ft_real :

#ifdef COERCE_EQUALITY 
   
	if (right->sp_form == ft_short) {
	
	   return ((left->sp_val.sp_real_ptr)->r_value ==
					(double)(right->sp_val.sp_short_value));
				
	} else if (right->sp_form == ft_long) {
	
			return ((left->sp_val.sp_real_ptr)->r_value ==
				(double)(long_to_double(SETL_SYSTEM right)));
				
	} else 	if (right->sp_form == ft_real) {

	   return ((left->sp_val.sp_real_ptr)->r_value ==
	           (right->sp_val.sp_real_ptr)->r_value);
	}
	return 0;
	
case ft_short:
	if (right->sp_form == ft_real) {
		
		return ((right->sp_val.sp_real_ptr)->r_value ==
					(double)(left->sp_val.sp_short_value));
	
	}
	return 0;
	break;
#else
	
	 return ((left->sp_val.sp_real_ptr)->r_value ==
	           (right->sp_val.sp_real_ptr)->r_value);

#endif
	 


/*\
 *  \case{long integers}
 *
 *  Long integers are equal if each cell is equal.
\*/

case ft_long :

{

#ifdef COERCE_EQUALITY
	if (right->sp_form == ft_real) {
		return ((right->sp_val.sp_real_ptr)->r_value ==
				(double)(long_to_double(SETL_SYSTEM left)));
	}

#endif
   /* pick out the root nodes */

   left_integer_hdr = left->sp_val.sp_long_ptr;
   right_integer_hdr = right->sp_val.sp_long_ptr;

   /* some easy tests -- check signs and number of cells */

   if (left_integer_hdr->i_is_negative != right_integer_hdr->i_is_negative)
      return NO;

   if (left_integer_hdr->i_cell_count != right_integer_hdr->i_cell_count)
      return NO;

   /* we have to traverse the cell list */

   for (left_integer_cell = left_integer_hdr->i_head,
           right_integer_cell = right_integer_hdr->i_head;
        left_integer_cell != NULL &&
           left_integer_cell->i_cell_value ==
           right_integer_cell->i_cell_value;
        left_integer_cell = left_integer_cell->i_next,
           right_integer_cell = right_integer_cell->i_next);

   if (left_integer_cell == NULL)
      return YES;
   else
      return NO;

}

/*\
 *  \case{strings}
 *
 *  Strings are equal if each cell is equal.
\*/

case ft_string :

{

   /* pick out the root nodes */

   left_string_hdr = left->sp_val.sp_string_ptr;
   right_string_hdr = right->sp_val.sp_string_ptr;

   /* an easy test -- check lengths */

   if (left_string_hdr->s_length != right_string_hdr->s_length)
      return NO;

   /* we have to traverse the cell list */

   string_length = left_string_hdr->s_length;
   for (left_string_cell = left_string_hdr->s_head,
           right_string_cell = right_string_hdr->s_head;
        left_string_cell != NULL;
        left_string_cell = left_string_cell->s_next,
           right_string_cell = right_string_cell->s_next) {

      string_cell_length = min(string_length,STR_CELL_WIDTH);
      for (left_char_ptr = left_string_cell->s_cell_value,
              right_char_ptr = right_string_cell->s_cell_value;
           left_char_ptr <
              left_string_cell->s_cell_value + string_cell_length;
           left_char_ptr++, right_char_ptr++) {

         if (*left_char_ptr != *right_char_ptr)
            return NO;

      }
      string_length -= string_cell_length;

   }

   return YES;

}

/*\
 *  \case{tuples}
 *
 *  Tuples are potentially expensive to test.  As a first cut, we check
 *  lengths and hash codes.  If this fails we have to check each cell.
\*/

case ft_tuple :

{

   /* pick out the root nodes */

   left_tuple_root = left->sp_val.sp_tuple_ptr;
   right_tuple_root = right->sp_val.sp_tuple_ptr;

   /* some easy tests -- length and hash code */

   if (left_tuple_root->t_ntype.t_root.t_length !=
       right_tuple_root->t_ntype.t_root.t_length)
      return NO;

   if (left_tuple_root->t_hash_code != right_tuple_root->t_hash_code)
      return NO;

   /* set up to iterate over the sources */

   left_tuple_work_hdr = left_tuple_root;
   left_number = -1;
   left_height = left_tuple_root->t_ntype.t_root.t_height;
   left_index = 0;
   left_element_needed = YES;

   right_tuple_work_hdr = right_tuple_root;
   right_number = -1;
   right_height = right_tuple_root->t_ntype.t_root.t_height;
   right_index = 0;
   right_element_needed = YES;

   for (;;) {

      if (left_element_needed) {

         /* find the next element in the tuple */

         left_element = NULL;
         for (;;) {

            /* if we have an element already, return it */

            if (!left_height && left_index < TUP_HEADER_SIZE) {

               if (left_tuple_work_hdr->t_child[left_index].t_cell == NULL) {

                  left_number++;
                  left_index++;

                  continue;

               }

               left_tuple_cell =
                  left_tuple_work_hdr->t_child[left_index].t_cell;
               left_element = &(left_tuple_cell->t_spec);

               left_number++;
               left_index++;

               break;

            }

            /* move up if we're at the end of a node */

            if (left_index >= TUP_HEADER_SIZE) {

               /* break if we've exhausted the source */

               if (left_tuple_work_hdr == left_tuple_root) {

                  left_number++;
                  break;

               }

               left_height++;
               left_index =
                  left_tuple_work_hdr->t_ntype.t_intern.t_child_index + 1;
               left_tuple_work_hdr =
                  left_tuple_work_hdr->t_ntype.t_intern.t_parent;

               continue;

            }

            /* skip over null nodes */

            if (left_tuple_work_hdr->t_child[left_index].t_header == NULL) {

               left_number += 1L << (left_height * TUP_SHIFT_DIST);
               left_index++;

               continue;

            }

            /* otherwise drop down a level */

            left_tuple_work_hdr =
               left_tuple_work_hdr->t_child[left_index].t_header;
            left_index = 0;
            left_height--;

         }
      }

      /* at this point we have a left element, or NULL */

      /* find a right element */

      if (right_element_needed) {

         /* find the next element in the set */

         right_element = NULL;
         for (;;) {

            /* if we have an element already, return it */

            if (!right_height && right_index < TUP_HEADER_SIZE) {

               if (right_tuple_work_hdr->t_child[right_index].t_cell ==
                   NULL) {

                  right_number++;
                  right_index++;

                  continue;

               }

               right_tuple_cell =
                  right_tuple_work_hdr->t_child[right_index].t_cell;
               right_element = &(right_tuple_cell->t_spec);

               right_number++;
               right_index++;

               break;

            }

            /* move up if we're at the end of a node */

            if (right_index >= TUP_HEADER_SIZE) {

               /* break if we've exhausted the rsource */

               if (right_tuple_work_hdr == right_tuple_root) {

                  right_number++;
                  break;

               }

               right_height++;
               right_index =
                  right_tuple_work_hdr->t_ntype.t_intern.t_child_index + 1;
               right_tuple_work_hdr =
                  right_tuple_work_hdr->t_ntype.t_intern.t_parent;

               continue;

            }

            /* skip over null nodes */

            if (right_tuple_work_hdr->t_child[right_index].t_header == NULL) {

               right_number += 1L << (right_height * TUP_SHIFT_DIST);
               right_index++;

               continue;

            }

            /* otherwise drop down a level */

            right_tuple_work_hdr =
               right_tuple_work_hdr->t_child[right_index].t_header;
            right_index = 0;
            right_height--;

         }
      }

      /* if we've exhausted both tuples they must be equal */

      if (left_number >= left_tuple_root->t_ntype.t_root.t_length &&
          right_number >= right_tuple_root->t_ntype.t_root.t_length)
         return YES;

      /* otherwise, check whether the item numbers match */

      if (left_number < right_number) {

         /*
          *  If the item numbers do not match, the lower one indicates a
          *  missing item if the item is NULL (there is nothing left in
          *  the tuple), but the higher one indicates a missing item
          *  otherwise.
          */

         if (left_element != NULL) {

            if (left_element->sp_form != ft_omega)
               return NO;

            left_element_needed = YES;
            right_element_needed = NO;

            continue;

         }

         if (right_element->sp_form != ft_omega)
            return NO;

         left_element_needed = NO;
         right_element_needed = YES;

         continue;

      }

      if (right_number < left_number) {

         if (right_element != NULL) {

            if (right_element->sp_form != ft_omega)
               return NO;

            right_element_needed = YES;
            left_element_needed = NO;

            continue;

         }

         if (left_element->sp_form != ft_omega)
            return NO;

         right_element_needed = NO;
         left_element_needed = YES;

         continue;

      }

      /* finally, we have two elements in the same position -- compare them */

      spec_equal(is_equal,left_element,right_element);

      if (!is_equal)
         return NO;

      left_element_needed = YES;
      right_element_needed = YES;

   }
}

/*\
 *  \case{maps}
 *
 *  Sets and maps are essentially an internal convenience -- the
 *  programmer isn't aware of the different representations.  If we must
 *  compare a set with a map we convert the map to a set and compare them
 *  as sets.  If we do have two maps we compare them as maps.
\*/

case ft_map :

{

   if (right->sp_form == ft_set) {

      map_to_set(SETL_SYSTEM &spare,left);
      left = &spare;

   }
   else {

      /* pick out the root nodes */

      left_map_root = left->sp_val.sp_map_ptr;
      right_map_root = right->sp_val.sp_map_ptr;

      /* we want the map with the greater height on the left */

      if (left_map_root->m_ntype.m_root.m_height <
          right_map_root->m_ntype.m_root.m_height) {

         left_map_work_hdr = left_map_root;
         left_map_root = right_map_root;
         right_map_root = left_map_work_hdr;

      }

      /* set up to loop over the left and right maps in parallel */

      left_map_work_hdr = left_map_root;
      left_height = left_map_root->m_ntype.m_root.m_height;
      left_index = 0;

      right_map_work_hdr = right_map_root;
      right_height = right_map_root->m_ntype.m_root.m_height;
      right_map_list = NULL;

      /* find successive clash lists, where right should contain left */

      for (;;) {

         /* find the next clash list */

         left_map_list = NULL;
         while (left_map_list == NULL) {

            /* return the clash list if we're at a leaf */

            if (!left_height && left_index < SET_HASH_SIZE) {

               left_map_list =
                  left_map_work_hdr->m_child[left_index].m_cell;

               /* if the right is also at a leaf, we set the right list */

               if (!right_height) {

                  right_map_list =
                     right_map_work_hdr->m_child[left_index].m_cell;

               }

               left_index++;

               continue;

            }

            /* move up if we're at the end of a node */

            if (left_index >= SET_HASH_SIZE) {

               if (left_map_work_hdr == left_map_root)
                  break;

               left_height++;
               left_index =
                  left_map_work_hdr->m_ntype.m_intern.m_child_index + 1;
               left_map_work_hdr =
                  left_map_work_hdr->m_ntype.m_intern.m_parent;

               /* move up on the right also */

               if (right_height >= 0) {

                  right_map_work_hdr =
                     right_map_work_hdr->m_ntype.m_intern.m_parent;
                  right_map_list = NULL;

               }

               right_height++;

               continue;

            }

            /* skip over null nodes */

            if (left_map_work_hdr->m_child[left_index].m_header == NULL) {

               left_index++;
               continue;

            }

            /* otherwise drop down a level */

            left_map_work_hdr =
               left_map_work_hdr->m_child[left_index].m_header;
            left_height--;

            /* drop down on the right, or return a right list */

            if (right_height > 0) {

               if (right_map_work_hdr->m_child[left_index].m_header == NULL) {

                  get_map_header(new_map_hdr);
                  new_map_hdr->m_ntype.m_intern.m_parent = right_map_work_hdr;
                  new_map_hdr->m_ntype.m_intern.m_child_index = left_index;
                  for (i = 0;
                       i < MAP_HASH_SIZE;
                       new_map_hdr->m_child[i++].m_cell = NULL);
                  right_map_work_hdr->m_child[left_index].m_header =
                     new_map_hdr;
                  right_map_work_hdr = new_map_hdr;

               }
               else {

                  right_map_work_hdr =
                     right_map_work_hdr->m_child[left_index].m_header;

               }
            }
            else if (!right_height) {

               right_map_list =
                  right_map_work_hdr->m_child[left_index].m_cell;

            }

            right_height--;
            left_index = 0;

         }

         /* break if we didn't find a list */

         if (left_map_list == NULL)
            break;

         /*
          *  At this point we have a clash list from each map.  The left
          *  lists will be unique for each iteration, but the right lists
          *  may repeat.  The right list will always contain those
          *  elements on the left which are in the right map, and perhaps
          *  some others.
          */

         /* loop over the left list */

         right_map_cell = right_map_list;
         for (left_map_cell = left_map_list;
              left_map_cell != NULL;
              left_map_cell = left_map_cell->m_next) {

            /* search for the element in the right list */

            while (right_map_cell != NULL &&
                   right_map_cell->m_hash_code < left_map_cell->m_hash_code)
               right_map_cell = right_map_cell->m_next;

            /* search through elements with identical hash codes */

            is_equal = NO;
            for (right_map_work_cell = right_map_cell;
                 right_map_work_cell != NULL &&
                    right_map_work_cell->m_hash_code ==
                       left_map_cell->m_hash_code &&
                    !is_equal;
                 right_map_work_cell = right_map_work_cell->m_next) {

               spec_equal(is_equal,
                          &(left_map_cell->m_domain_spec),
                          &(right_map_work_cell->m_domain_spec));
               save_map_work_cell = right_map_work_cell;

            }

            /* if we didn't find the element in the right map, return NO */

            if (!is_equal) {

               unmark_specifier(&spare);
               return NO;

            }

            if (save_map_work_cell == NULL)
               continue;

            if (left_map_cell->m_is_multi_val !=
                save_map_work_cell->m_is_multi_val) {

               unmark_specifier(&spare);
               return NO;

            }

            /* we found equal domain elements, check the range */

            spec_equal(is_equal,
                       &(left_map_cell->m_range_spec),
                       &(save_map_work_cell->m_range_spec));

            if (!is_equal) {

               unmark_specifier(&spare);
               return NO;

            }
         }
      }

      /* if we break, left_map is a subset of right_map */

      unmark_specifier(&spare);
      return YES;

   }
}

/*\
 *  \case{sets}
 *
 *  Sets and maps are essentially an internal convenience -- the
 *  programmer isn't aware of the different representations.  If we must
 *  Accordingly, we might have to convert the right operand to a set.
 *  Then we loop over the elements of each set in parallel comparing
 *  them.
\*/

case ft_set :

{

   /* pick out the root pointers */

   left_set_root = left->sp_val.sp_set_ptr;
   if (right->sp_form == ft_map) {

      map_to_set(SETL_SYSTEM &spare,right);
      right_set_root = spare.sp_val.sp_set_ptr;

   }
   else {

      right_set_root = right->sp_val.sp_set_ptr;

   }

   /* some easy tests -- cardinality and hash code */

   if (left_set_root->s_ntype.s_root.s_cardinality !=
       right_set_root->s_ntype.s_root.s_cardinality) {

      unmark_specifier(&spare);
      return NO;

   }

   if (left_set_root->s_hash_code != right_set_root->s_hash_code) {

      unmark_specifier(&spare);
      return NO;

   }

   /* we want the set with the greater height on the left */

   if (left_set_root->s_ntype.s_root.s_height <
       right_set_root->s_ntype.s_root.s_height) {

      left_set_work_hdr = left_set_root;
      left_set_root = right_set_root;
      right_set_root = left_set_work_hdr;

   }

   /* set up to loop over the left and right sets in parallel */

   left_set_work_hdr = left_set_root;
   left_height = left_set_root->s_ntype.s_root.s_height;
   left_index = 0;

   right_set_work_hdr = right_set_root;
   right_height = right_set_root->s_ntype.s_root.s_height;
   right_set_list = NULL;

   /* find successive clash lists, where the right should contain the left */

   for (;;) {

      /* find the next clash list */

      left_set_list = NULL;
      while (left_set_list == NULL) {

         /* return the clash list if we're at a leaf */

         if (!left_height && left_index < SET_HASH_SIZE) {

            left_set_list =
               left_set_work_hdr->s_child[left_index].s_cell;

            /* if the right is also at a leaf, we set the right list */

            if (!right_height) {

               right_set_list =
                  right_set_work_hdr->s_child[left_index].s_cell;

            }

            left_index++;

            continue;

         }

         /* move up if we're at the end of a node */

         if (left_index >= SET_HASH_SIZE) {

            if (left_set_work_hdr == left_set_root)
               break;

            left_height++;
            left_index =
               left_set_work_hdr->s_ntype.s_intern.s_child_index + 1;
            left_set_work_hdr =
               left_set_work_hdr->s_ntype.s_intern.s_parent;

            /* move up on the right also */

            if (right_height >= 0) {

               right_set_work_hdr =
                  right_set_work_hdr->s_ntype.s_intern.s_parent;
               right_set_list = NULL;

            }

            right_height++;

            continue;

         }

         /* skip over null nodes */

         if (left_set_work_hdr->s_child[left_index].s_header == NULL) {

            left_index++;
            continue;

         }

         /* otherwise drop down a level */

         left_set_work_hdr =
            left_set_work_hdr->s_child[left_index].s_header;
         left_height--;

         /* drop down on the right, or return a right list */

         if (right_height > 0) {

            if (right_set_work_hdr->s_child[left_index].s_header == NULL) {

               get_set_header(new_set_hdr);
               new_set_hdr->s_ntype.s_intern.s_parent = right_set_work_hdr;
               new_set_hdr->s_ntype.s_intern.s_child_index = left_index;
               for (i = 0;
                    i < SET_HASH_SIZE;
                    new_set_hdr->s_child[i++].s_cell = NULL);
               right_set_work_hdr->s_child[left_index].s_header =
                  new_set_hdr;
               right_set_work_hdr = new_set_hdr;

            }
            else {

               right_set_work_hdr =
                  right_set_work_hdr->s_child[left_index].s_header;

            }
         }
         else if (!right_height) {

            right_set_list =
               right_set_work_hdr->s_child[left_index].s_cell;

         }

         right_height--;
         left_index = 0;

      }

      /* break if we didn't find a list */

      if (left_set_list == NULL)
         break;

      /*
       *  At this point we have a clash list from each set.  The left
       *  lists will be unique for each iteration, but the right lists
       *  may repeat.  The right list will always contain those elements
       *  on the left which are in the right set, and perhaps some
       *  others.
       */

      /* loop over the left list */

      right_set_cell = right_set_list;
      for (left_set_cell = left_set_list;
           left_set_cell != NULL;
           left_set_cell = left_set_cell->s_next) {

         /* search for the element in the right list */

         while (right_set_cell != NULL &&
                right_set_cell->s_hash_code < left_set_cell->s_hash_code)
            right_set_cell = right_set_cell->s_next;

         /* search through elements with identical hash codes */

         is_equal = NO;
         for (right_set_work_cell = right_set_cell;
              right_set_work_cell != NULL &&
                 right_set_work_cell->s_hash_code ==
                    left_set_cell->s_hash_code &&
                 !is_equal;
              right_set_work_cell = right_set_work_cell->s_next) {

            spec_equal(is_equal,
                       &(left_set_cell->s_spec),
                       &(right_set_work_cell->s_spec));

         }

         /* if we didn't find the element in the right set, return NO */

         if (!is_equal) {

            unmark_specifier(&spare);
            return NO;

         }
      }
   }

   /* if we break, left_set is a subset of right_set */

   unmark_specifier(&spare);
   return YES;

}

/*\
 *  \case{objects}
 *
 *  Tuples are potentially expensive to test.  As a first cut, we check
 *  lengths and hash codes.  If this fails we have to check each cell.
\*/

case ft_object :

{

   /* pick out the root nodes */

   left_object_root = left->sp_val.sp_object_ptr;
   right_object_root = right->sp_val.sp_object_ptr;

   /* some easy tests -- class and hash code */

   if (left_object_root->o_ntype.o_root.o_class !=
       right_object_root->o_ntype.o_root.o_class)
      return NO;

   if (left_object_root->o_hash_code != right_object_root->o_hash_code)
      return NO;

   /* set up to iterate over the sources */

   class_ptr = left_object_root->o_ntype.o_root.o_class;
   left_object_work_hdr = left_object_root;
   left_number = 0;
   left_height = class_ptr->ut_obj_height;
   left_index = 0;
   right_object_work_hdr = right_object_root;

   for (left_number = 0;
        left_number < class_ptr->ut_var_count;
        left_number++) {

      /* drop down to a leaf */

      while (left_height) {

         /* extract the element's index at this level */

         left_index = (left_number >>
                              (left_height * OBJ_SHIFT_DIST)) &
                           OBJ_SHIFT_MASK;

         /* drop down one level */

         left_object_work_hdr =
             left_object_work_hdr->o_child[left_index].o_header;
         right_object_work_hdr =
             right_object_work_hdr->o_child[left_index].o_header;
         left_height--;

      }

      /*
       *  At this point, object_work_hdr points to the lowest level header
       *  record.  We swap elements in the appropriate slot.
       */

      left_index = left_number & OBJ_SHIFT_MASK;
      left_element = &((left_object_work_hdr->
                         o_child[left_index].o_cell)->o_spec);
      right_element = &((right_object_work_hdr->
                         o_child[left_index].o_cell)->o_spec);

      spec_equal(is_equal,left_element,right_element);

      if (!is_equal)
         return NO;

      /* if we've finished an internal node, move up */

      left_index++;
      while (left_index >= OBJ_HEADER_SIZE) {

         left_height++;
         left_index =
            left_object_work_hdr->o_ntype.o_intern.o_child_index + 1;
         left_object_work_hdr =
            left_object_work_hdr->o_ntype.o_intern.o_parent;
         right_object_work_hdr =
            right_object_work_hdr->o_ntype.o_intern.o_parent;

      }
   }

   return YES;

}

#ifdef TRAPS

   default :

      trap(__FILE__,__LINE__,msg_bad_spec_eq_call);

#endif

   /* return to normal indentation */

   }

   return 0;  /* placate semantic error checker */

}

/*\
 *  \function{spec\_hash\_code()}
 *
 *  This macro tries to find hash codes.  If we are lucky we can return
 *  the hash code directly.  If we are not lucky we call a function to
 *  calculate the hash code.
\*/

int32 spec_hash_code_mac(
   specifier *s)

{

   if ((s)->sp_form == ft_omega)
      return 0L;
   else if ((s)->sp_form <= ft_short)
      return (s)->sp_val.sp_short_value; 
   else if ((s)->sp_form <= ft_iter)
      return (int32)((s)->sp_val.sp_biggest); 
   else if ((s)->sp_form >= ft_tuple)
      return (*((int32 *)((s)->sp_val.sp_biggest) + 1)); 
   else 
      return spec_hash_code_calc(s); 

}

/*\
 *  \function{spec\_hash\_code\_calc()}
 *
 *  This function is a second level hash code calculation.  Easy cases
 *  are handled by \verb"spec_hash_code".
\*/

int32 spec_hash_code_calc(
   specifier *element)                 /* set element to be hashed          */

{
int32 set_hash_code;                   /* hash code eventually returned     */
integer_c_ptr_type integer_ptr;        /* integer cell pointer              */
string_c_ptr_type string_ptr;          /* string cell pointer               */
int32 string_length;                   /* work string length                */
unsigned top_four;                     /* top four bits of hash code        */
char *p;                               /* temporary looping variable        */
#define MASK (0x0f << (sizeof(unsigned) * 8 - 4))
                                       /* bit string with four high order   */
                                       /* bits of integer on, others off    */
#define SHIFT (sizeof(unsigned) * 8 - 8);
                                       /* shift distance                    */

   /* we use one of a variety of hash functions */

   switch (element->sp_form) {

      /*
       *  The hash code of a long integer is just the low order n bits of
       *  the integer, where n is the number of bits in an unsigned
       *  long.  We have to visit three cells to gather up n bits.
       */

      case ft_long :

         /* if we've already calculated the hash code, just return it */

         if ((element->sp_val.sp_string_ptr)->s_hash_code >= 0)
            return (element->sp_val.sp_string_ptr)->s_hash_code;

         /* otherwise, calculate and set the hash code */

         integer_ptr = (element->sp_val.sp_long_ptr)->i_head;
         set_hash_code = integer_ptr->i_cell_value;
         integer_ptr = integer_ptr->i_next;

         if (integer_ptr != NULL) {

            set_hash_code |= (integer_ptr->i_cell_value << INT_CELL_WIDTH);
            integer_ptr = integer_ptr->i_next;

            if (integer_ptr != NULL)
               set_hash_code |=
                  integer_ptr->i_cell_value << (2 * INT_CELL_WIDTH);

         }

         /* save the hash code in case it's needed again */

         (element->sp_val.sp_string_ptr)->s_hash_code = set_hash_code;

         return set_hash_code;

      /*
       *  It's difficult to come up with a good machine-independent hash
       *  code for real numbers.  We use different schemes for each
       *  machine / compiler.
       */

      case ft_real :

         /*
          *  Microsoft C uses IEEE long values for doubles, but the Intel
          *  processor puts least significant bits on the left.  We are
          *  trying to use the mantissa, but reversed, so the most
          *  significant bits of the mantissa are on the right.  This
          *  isn't perfect, but it's close.
          */

#if IEEE_LEND

         p = (char *)&((element->sp_val.sp_real_ptr)->r_value);
         return ((int32)(*(p + 6)) & 0x0f) |
                ((int32)(*(p + 5)) << 4) |
                ((int32)(*(p + 4)) << 12) |
                ((int32)(*(p + 3)) << 20) |
                ((int32)((*(p + 2)) & 0x0f) << 28);

#endif
#if IEEE_BEND

         p = (char *)&((element->sp_val.sp_real_ptr)->r_value);
         return ((int32)(*(p + 1)) & 0x0f) |
                ((int32)(*(p + 2)) << 4) |
                ((int32)(*(p + 3)) << 12) |
                ((int32)(*(p + 4)) << 20) |
                ((int32)((*(p + 5)) & 0x0f) << 28);

#endif
#if G_FLOAT

         p = (char *)&((element->sp_val.sp_real_ptr)->r_value);
         return ((int32)(*(p + 1)) & 0x0f) |
                ((int32)(*(p + 2)) << 4) |
                ((int32)(*(p + 3)) << 12) |
                ((int32)(*(p + 4)) << 20) |
                ((int32)((*(p + 5)) & 0x0f) << 28);

#endif
#if D_FLOAT

         p = (char *)&((element->sp_val.sp_real_ptr)->r_value);
         return ((int32)(*(p + 1)) & 0x7f) |
                ((int32)(*(p + 2)) << 7) |
                ((int32)(*(p + 3)) << 15) |
                ((int32)(*(p + 4)) << 23) |
                ((int32)((*(p + 5)) & 0x01) << 31);

#endif

      /*
       *  The hash code of a string uses an algorithm due to P. J.
       *  Weinstein which can be found in Compilers: Principles,
       *  Techniques, and Tools by Aho, Sethi and Ullman.
       */

      case ft_string :

         /* if we've already calculated the hash code, just return it */

         if ((element->sp_val.sp_string_ptr)->s_hash_code >= 0)
            return (element->sp_val.sp_string_ptr)->s_hash_code;

         /* otherwise, calculate and set the hash code */

         set_hash_code = 0;
         string_length = (element->sp_val.sp_string_ptr)->s_length;
         for (string_ptr = (element->sp_val.sp_string_ptr)->s_head;
              string_length > 0;
              string_ptr = string_ptr->s_next) {

            for (p = string_ptr->s_cell_value;
                 string_length > 0 &&
                    p < string_ptr->s_cell_value + STR_CELL_WIDTH;
                 p++, string_length--) {

               set_hash_code = (set_hash_code << 4) + *p;
               if (top_four = set_hash_code & MASK) {
                  set_hash_code ^= top_four >> SHIFT;
                  set_hash_code ^= top_four;
               }
            }
         }

         /* save the hash code in case it's needed again */

         (element->sp_val.sp_string_ptr)->s_hash_code = set_hash_code;

         return set_hash_code;

   }

#ifdef TRAPS

   trap(__FILE__,__LINE__,msg_bad_form_hash);

#endif

   return 0;  /* placate semantic error checker */

}
