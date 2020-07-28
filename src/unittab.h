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
 *  \packagespec{Unit Table}
\*/

#ifndef UNITTAB_LOADED

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */

struct slot_info_item {
   unsigned si_in_class : 1;           /* YES if used in class              */
   unsigned si_is_method : 1;          /* slot type                         */
   unsigned si_is_public : 1;          /* YES if public slot                */
   struct slot_item *si_slot_ptr;      /* slot name pointer                 */
   struct specifier_item *si_spec;     /* slot specifier location           */
   int32 si_index;                     /* index in instance (object)        */
   struct slot_info_item *si_next_var; /* next instance variable            */
};

/* unit table item structure */

struct unittab_item {
   int ut_type;                        /* unit type                         */
   struct unittab_item *ut_hash_link;  /* next unit with same hash value    */
   char *ut_name;                      /* unit lexeme                       */
   struct unittab_item *ut_parent;     /* parent in unit tree               */
   struct unittab_item **ut_unit_tab;  /* imported unit table               */
   int32 ut_units_loaded;              /* ancestors loaded so far           */
   int32 ut_last_inherit;              /* end of inherited units list       */
   struct instruction_item *ut_body_code;
                                       /* unit body code pointer            */
   struct instruction_item *ut_init_code;
                                       /* unit initialization code pointer  */
   struct specifier_item *ut_data_ptr; /* unit data pointer                 */
   struct slot_info_item *ut_slot_info;
                                       /* slot information array            */
   int32 ut_slot_count;                /* number of slots in array          */
   struct slot_info_item *ut_first_var;
                                       /* first instance variable           */
   int32 ut_var_count;                 /* number of instance variables      */
   int ut_obj_height;                  /* height of object tree             */
   struct self_stack_item *ut_self;    /* current self for class            */
   char ut_source_name[PATH_LENGTH + 1];
                                       /* unit source file name             */
   time_t ut_time_stamp;               /* time unit was compiled            */
   int ut_is_loaded;                   /* YES if the unit has been loaded   */
   int ut_current_saved;               /* YES if already saved during       */
                                       /* swap                              */
   void *ut_native_code;               /* Handle to DLL containing code     */
   struct map_h_item *ut_symbol_map;   /* map from name to procedure        */
   struct specifier_item *ut_err_ext_map;
   long ut_nlines;                     /* Number of instructions            */
   struct profiler_item *ut_prof_table;
                                       /* Pointer to profiler table         */
   struct unittab_item *ut_next;       /* next unit loaded                  */
   
};

typedef struct unittab_item *unittab_ptr_type;
                                       /* unit table type definition        */

/* clear a unit table item */

#define clear_unittab(u) { \
   (u)->ut_type = -1;                  (u)->ut_hash_link = NULL; \
   (u)->ut_parent = NULL;              (u)->ut_units_loaded = 0; \
   (u)->ut_last_inherit = 0; \
   (u)->ut_name = NULL;                (u)->ut_body_code = NULL; \
   (u)->ut_init_code = NULL;           (u)->ut_data_ptr = NULL; \
   (u)->ut_slot_info = NULL; \
   (u)->ut_source_name[0] = '\0';      (u)->ut_time_stamp = 0; \
   (u)->ut_is_loaded = 0; (u)->ut_native_code = NULL; \
   (u)->ut_nlines=0; (u)->ut_prof_table=NULL; (u)->ut_next=NULL; \
}

/* public function declarations */

void init_unittab(SETL_SYSTEM_PROTO_VOID);            
                                       /* initialize unit table             */
unittab_ptr_type get_unittab(SETL_SYSTEM_PROTO char *);  
                                       /* get unit table item               */

/* performance tuning constants */
#define UNITTAB__HASH_TABLE_SIZE    13 /* size of hash table                */

#define UNITTAB_LOADED 1
#endif
