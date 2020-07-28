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
 *  \package{The Unit Loader}
 *
 *  This package is responsible for loading a unit from a library into
 *  the various memory tables.
 *
 *  \texify{loadunit.h}
 *
 *  \packagebody{Unit Loader}
\*/

/* standard C header files */

#include <math.h>                      /* dynamic memory allocation         */
#include <assert.h>

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "mcode.h"                     /* method codes                      */
#include "pcode.h"                     /* pseudo code                       */
#include "libman.h"                    /* library manager                   */
#include "builtins.h"                  /* built-in symbols                  */
#include "unittab.h"                   /* unit table                        */
#include "loadunit.h"                  /* unit loader                       */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_reals.h"                   /* real numbers                      */
#include "x_strngs.h"                  /* strings                           */
#include "maps.h"                      /* maps                              */
#include "procs.h"                     /* procedures                        */
#include "objects.h"                   /* objects                           */
#include "slots.h"                     /* procedures                        */

#ifdef MACINTOSH
#include <Types.h>
#include <Files.h>
#include <PPCToolBox.h>
#include <TextUtils.h>
#include <CodeFragments.h>
#endif /* MACINTOSH */

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef PLUGIN
#define fprintf plugin_fprintf
#define printf  plugin_printf
#endif

#ifdef WIN32
#include <windows.h>
#endif

/* constants */

#define EVAL_PACK    "EVAL_VARS"
#define EVAL_NUMS    1000

/* forward declarations */

static void load_specifiers(SETL_SYSTEM_PROTO
                            unittab_ptr_type,
                            libunit_ptr_type,
                            unit_control_record *);

static instruction *load_pcode(SETL_SYSTEM_PROTO
                               libstr_ptr_type,
                               unittab_ptr_type,
                               slot_ptr_type *,
                               int32);

static void load_public(SETL_SYSTEM_PROTO unittab_ptr_type, libunit_ptr_type);

struct connection {
   char *key;
   void *connID;
};

typedef struct connection connection_record;

static connection_record libraries[128];
static libraries_loaded=0;

/*\
 *  \function{load\_unit()}
 *
 *  This function loads a unit from the libraries into the various memory
 *  structures.
\*/

unittab_ptr_type load_unit(
   SETL_SYSTEM_PROTO
   char *unit_name,                    /* name of unit to be loaded         */
   unittab_ptr_type unit_parent,       /* parent pointer                    */
   char *base_name)                    /* base for inherited units          */

{
unittab_ptr_type unittab_ptr;          /* returned unit pointer             */
unittab_ptr_type temp_parent;          /* parent pointer (looping)          */
libunit_ptr_type libunit_ptr;          /* current unit                      */
libstr_ptr_type libstr_ptr;            /* library stream pointer            */
unit_control_record unit_control;      /* unit control record               */
slot_record slot;                      /* slot record                       */
char name_buffer[MAX_TOK_LEN];         /* name buffer for slot              */
slot_ptr_type *slot_ptr_tab, slot_ptr; /* pointer for each slot             */
int32 work_length;                     /* working length (destroyed)        */
import_record import;                  /* imported unit record              */
unittab_ptr_type import_unit;          /* pointer to imported unit          */
int32 i,j;                             /* temporary looping variables       */
struct slot_info_item **next_var;      /* next instance variable            */
int32 slot_index;                      /* slot index within instance        */
instruction *save_pc;                  /* saved instruction pointer         */
instruction *save_ip;                  /* saved instruction pointer         */
long line;                             /* temporary looping variable        */
struct profiler_item *pi;              /* temporary looping variable        */
char *key;
int32 initok;




#ifdef MACINTOSH
OSErr myErr;
FSSpec myFSSpec;
CFragConnectionID myConnID;
Ptr myMainAddr;
Ptr myAddr;
CFragSymbolClass myClass;
Str255 myErrName;
Str63 fragName;
#endif

#ifdef WIN32
HINSTANCE hDLL;
void *psymb;
#endif

#if HAVE_DLFCN_H
void *handle;
void *psymb;
#endif


   /*
    *  First open a new unit table record.  We form the unit name by
    *  prepending the base name, then look for the unit in the unit
    *  table.  If we find it already, then return.  Otherwise we have to
    *  load the unit.
    */

   if (base_name != NULL)
      sprintf(name_buffer,"%s:%s",base_name,unit_name);
   else
      strcpy(name_buffer,unit_name);
   unittab_ptr = get_unittab(SETL_SYSTEM name_buffer);

   /* update ancestors' unit tables */

   for (temp_parent = unit_parent;
        temp_parent != NULL;
        temp_parent = temp_parent->ut_parent) {

      temp_parent->ut_unit_tab[temp_parent->ut_units_loaded++] = unittab_ptr;
      if (unittab_ptr->ut_is_loaded) {
         for (i = 2; i <= unittab_ptr->ut_last_inherit; i++) {
            temp_parent->ut_unit_tab[temp_parent->ut_units_loaded++] =
               unittab_ptr->ut_unit_tab[i];
         }
      }
   }

   /* if we were previously loaded, don't load us again */

   if (unittab_ptr->ut_is_loaded)
      return unittab_ptr;

#ifdef DEBUG
   if ((TRACING_ON)&&(!PROF_DEBUG)) {
      fprintf(DEBUG_FILE, "Loading %s\n", unit_name); fflush(DEBUG_FILE);
   }
   if (head_unittab==NULL) head_unittab=last_unittab=unittab_ptr;
   else {
     if (last_unittab!=unittab_ptr) {
          last_unittab->ut_next=unittab_ptr;
         last_unittab=unittab_ptr;
     }
   }
#endif /* DEBUG */

   unittab_ptr->ut_is_loaded = YES;
   unittab_ptr->ut_self = NULL;
   unittab_ptr->ut_err_ext_map=NULL;

   /* find the unit in the library */

   libunit_ptr = open_libunit(SETL_SYSTEM unit_name,NULL,LIB_READ_UNIT);
   if (libunit_ptr == NULL)
      return NULL;

   /* load the unit control record */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_CONTROL_STREAM);
   read_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
               sizeof(unit_control_record));
   close_libstr(SETL_SYSTEM libstr_ptr);

   /* make sure package bodies are compiled */

   if (unit_control.uc_needs_body)
      giveup(SETL_SYSTEM msg_package_uncompiled,unit_name);

   /* start filling in the unit table record */

   unittab_ptr->ut_type = unit_control.uc_type;
   
   if (unit_control.uc_type==NATIVE_UNIT) {
      key = (char *)malloc(strlen(unittab_ptr->ut_name)+
                strlen(SO_EXTENSION)+strlen(NATIVE_INIT)+PATH_LENGTH+16);
      if (key == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      sprintf(key,"%s%s%s",setl2_shlib_path,unittab_ptr->ut_name,SO_EXTENSION);
      
      for (i=0;i<libraries_loaded;i++) {
         if (strcmp(libraries[i].key,key)==0) {
             unittab_ptr->ut_native_code = (void *)libraries[i].connID;
             i=-1;
         	 break;
         }
      
      }
      if (i==-1) {
		//printf("Package %s already loaded...\n",key);
      	
      } else {
        //printf("Opening the package %s\n",key);
	  	libraries[libraries_loaded].key=malloc(strlen(key)+1);
		if (libraries[libraries_loaded].key == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
        strcpy(libraries[libraries_loaded].key,key);   

#ifdef MACINTOSH

		

      
        myErr = GetSharedLibrary (c2pstr(key),
                                kPowerPCCFragArch,
                                kReferenceCFrag,
                                &myConnID,
                                &myMainAddr,
                               myErrName);
        if (!myErr) {
           unittab_ptr->ut_native_code = (void *)myConnID;
           //printf("The connection id is %d\n",(int)(unittab_ptr->ut_native_code));
        
        }
        else {
        	//printf("The error is (%ld) %s \n",myErr,p2cstr(myErrName));
           giveup(SETL_SYSTEM msg_native_lib_open_error,unittab_ptr->ut_name);
        }
#endif

#if HAVE_DLFCN_H
        handle=dlopen(key,RTLD_LAZY);
        if (handle!=NULL)
           unittab_ptr->ut_native_code = (void *)handle;
        else  {
          printf("ERROR: %s\n",dlerror());        
          giveup(SETL_SYSTEM msg_native_lib_open_error,unittab_ptr->ut_name);
        }
#endif

#ifdef WIN32
	    hDLL = LoadLibrary(key);
	    if (hDLL!=NULL) 
			unittab_ptr->ut_native_code = (void *)hDLL;
	    else  {
			CHAR szBuf[80]; 
		    DWORD dw = GetLastError(); 
 
			sprintf(szBuf, "LoadLibrary failed: GetLastError returned %u\n", dw); 
			fprintf(stderr,szBuf);
			giveup(SETL_SYSTEM "Error loading %s DLL \n",key);
		}
#endif
	    
		libraries[libraries_loaded].connID=unittab_ptr->ut_native_code;
	    libraries_loaded++;
	  }

      sprintf(key,"%s%s",unittab_ptr->ut_name,NATIVE_INIT);
  
      /* Now call the Native package initialization */

#ifdef MACINTOSH
         myErr = FindSymbol((CFragConnectionID)unittab_ptr->ut_native_code,
                            c2pstr(key),
                            &myAddr, &myClass);

         if (!myErr)
            initok=(*(int32(*)(SETL_SYSTEM_PROTO_VOID))myAddr)(SETL_SYSTEM_VOID);
#endif


#if HAVE_DLFCN_H
         psymb=dlsym(unittab_ptr->ut_native_code,key);
         if (psymb!=NULL)
            initok=(*(int32(*)(SETL_SYSTEM_PROTO_VOID))psymb)(SETL_SYSTEM_VOID);
#endif

#ifdef WIN32
	     psymb = GetProcAddress((HINSTANCE)(unittab_ptr->ut_native_code), key);
	     if (psymb!=NULL)
            initok=(*(int32(*)(SETL_SYSTEM_PROTO_VOID))psymb)(SETL_SYSTEM_VOID);
		 
#endif

      free(key);
   }

   strcpy(unittab_ptr->ut_source_name,unit_control.uc_spec_source_name);
   unittab_ptr->ut_time_stamp = unit_control.uc_time_stamp;

   /* allocate space for specifiers */

#ifdef DYNAMIC_COMP
   if (strcmp(unit_name,EVAL_PACK)==0) {
      unittab_ptr->ut_data_ptr =
         get_specifiers(SETL_SYSTEM EVAL_NUMS);

   } else
#endif
   unittab_ptr->ut_data_ptr =
      get_specifiers(SETL_SYSTEM unit_control.uc_spec_count + 1);

   /* allocate a unit table */

   unittab_ptr->ut_unit_tab = (unittab_ptr_type *)malloc(
      (size_t)((unit_control.uc_unit_count + 2) *
               sizeof(unittab_ptr_type *)));
   if (unittab_ptr->ut_unit_tab == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   unittab_ptr->ut_unit_tab[0] = get_unittab(SETL_SYSTEM "$predefined");
   unittab_ptr->ut_unit_tab[1] = unittab_ptr;
   unittab_ptr->ut_units_loaded = 2;

   /* load units imported with an 'inherit' clause (propagate back) */

   unittab_ptr->ut_parent = unit_parent;
   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INHERIT_STREAM);

   for (i = 0; i < unit_control.uc_inherit_count; i++) {

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&import,sizeof(import_record));
      if (base_name != NULL)
         import_unit = load_unit(SETL_SYSTEM import.ir_name,unittab_ptr,base_name);
      else
         import_unit = load_unit(SETL_SYSTEM import.ir_name,unittab_ptr,unit_name);

      if (import_unit->ut_type != CLASS_UNIT)
         giveup(SETL_SYSTEM "Expected %s to be a class",
                import.ir_name);

      if (strcmp(import_unit->ut_source_name,import.ir_source_name) != 0 ||
          import_unit->ut_time_stamp != import.ir_time_stamp)
         giveup(SETL_SYSTEM msg_package_needs_compiled,unit_name);

   }

   close_libstr(SETL_SYSTEM libstr_ptr);

   /* load units imported with a 'use' clause (don't propagate) */

   unittab_ptr->ut_last_inherit = unittab_ptr->ut_units_loaded - 1;
   unittab_ptr->ut_parent = NULL;
   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_IMPORT_STREAM);

   for (i = 0; i < unit_control.uc_import_count; i++) {

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&import,sizeof(import_record));
      import_unit = load_unit(SETL_SYSTEM import.ir_name,unittab_ptr,NULL);

      if (import_unit->ut_type == PROGRAM_UNIT)
         giveup(SETL_SYSTEM msg_expected_pack_not_unit,
                import.ir_name);

      if (strcmp(import_unit->ut_source_name,import.ir_source_name) != 0 ||
          import_unit->ut_time_stamp != import.ir_time_stamp)
         giveup(SETL_SYSTEM msg_package_needs_compiled,unit_name);

   }

   close_libstr(SETL_SYSTEM libstr_ptr);
   unittab_ptr->ut_parent = unit_parent;

   /* build the slot table */

   slot_ptr_tab = (slot_ptr_type *)malloc(
      (size_t)((unit_control.uc_max_slot + 2) * sizeof(slot_ptr_type)));

   /*
    *  We make two passes over the slot stream.  The first pass just adds
    *  all the slot names to the slot table.  We have to do this first to
    *  build an array relating internal slot numbers to global slot
    *  numbers.
    */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_DSLOT_STREAM);

   for (i = 0; i < unit_control.uc_slot_count; i++) {

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&slot,sizeof(slot_record));

      read_libstr(SETL_SYSTEM libstr_ptr,
                  name_buffer,
                  (size_t)slot.sl_name_length);

      name_buffer[slot.sl_name_length] = '\0';

      slot_ptr = get_slot(SETL_SYSTEM name_buffer);
      slot_ptr_tab[slot.sl_number] = slot_ptr;

   }

   close_libstr(SETL_SYSTEM libstr_ptr);

   /*
    *  Now we start the second pass.  The goal here is to build up the
    *  slot information array in the unit table.  We only have to do this
    *  for classes imported with a 'use' clause.
    */

   if ((unittab_ptr->ut_type == CLASS_UNIT ||
        unittab_ptr->ut_type == PROCESS_UNIT) &&
       base_name == NULL) {

      /* build the slot table */

      unittab_ptr->ut_slot_count = TOTAL_SLOT_COUNT;
      unittab_ptr->ut_slot_info = (struct slot_info_item *)malloc(
         (size_t)(TOTAL_SLOT_COUNT * sizeof(struct slot_info_item)));
      unittab_ptr->ut_first_var = NULL;
      next_var = &(unittab_ptr->ut_first_var);
      unittab_ptr->ut_var_count = 0;
      slot_index = 0;

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_DSLOT_STREAM);

      for (j = 0; j < TOTAL_SLOT_COUNT; j++)
         unittab_ptr->ut_slot_info[j].si_in_class = NO;

      for (i = 0; i < unit_control.uc_slot_count; i++) {

         read_libstr(SETL_SYSTEM libstr_ptr,(char *)&slot,sizeof(slot_record));

         read_libstr(SETL_SYSTEM libstr_ptr,
                     name_buffer,
                     (size_t)slot.sl_name_length);

         name_buffer[slot.sl_name_length] = '\0';

         j = slot_ptr_tab[slot.sl_number]->sl_number;

         /*
          *  MPW C doesn't seem to be able to copy bit fields.  I hope I
          *  found all the places I do that!
          */

#ifdef MPWC

{
int mpw_bug_workaround;

         mpw_bug_workaround = slot.sl_is_method;
         unittab_ptr->ut_slot_info[j].si_is_method = mpw_bug_workaround;
         mpw_bug_workaround = slot.sl_is_public;
         unittab_ptr->ut_slot_info[j].si_is_public = mpw_bug_workaround;
         mpw_bug_workaround = slot.sl_in_class;
         unittab_ptr->ut_slot_info[j].si_in_class = mpw_bug_workaround;

}
#else

         unittab_ptr->ut_slot_info[j].si_is_method = slot.sl_is_method;
         unittab_ptr->ut_slot_info[j].si_is_public = slot.sl_is_public;
         unittab_ptr->ut_slot_info[j].si_in_class = slot.sl_in_class;

#endif

         unittab_ptr->ut_slot_info[j].si_slot_ptr =
            slot_ptr_tab[slot.sl_number];
         unittab_ptr->ut_slot_info[j].si_spec =
            (unittab_ptr->ut_unit_tab[slot.sl_unit_num])->ut_data_ptr +
               slot.sl_offset;
         if (slot.sl_in_class && !slot.sl_is_method) {
            unittab_ptr->ut_slot_info[j].si_index = slot_index++;
            *next_var = unittab_ptr->ut_slot_info + j;
            next_var = &(unittab_ptr->ut_slot_info[j].si_next_var);
            *next_var = NULL;
            unittab_ptr->ut_var_count++;
         }
         else {
            unittab_ptr->ut_slot_info[j].si_index = -1;
         }
      }

      /* calculate height of header tree */

      work_length = unittab_ptr->ut_var_count;
      unittab_ptr->ut_obj_height = 0;
      while (work_length = work_length >> OBJ_SHIFT_DIST)
         unittab_ptr->ut_obj_height++;

      close_libstr(SETL_SYSTEM libstr_ptr);

   }

   /* load the initialization code */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INIT_STREAM);
   unittab_ptr->ut_init_code =
      load_pcode(SETL_SYSTEM libstr_ptr,
                 unittab_ptr,
                 slot_ptr_tab,
                 unit_control.uc_ipcode_count);
   close_libstr(SETL_SYSTEM libstr_ptr);

   /* load the body code */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_PCODE_STREAM);
   unittab_ptr->ut_body_code =
      load_pcode(SETL_SYSTEM libstr_ptr,
                 unittab_ptr,
                 slot_ptr_tab,
                 unit_control.uc_bpcode_count);
   close_libstr(SETL_SYSTEM libstr_ptr);

#ifdef DEBUG
   if (PROF_DEBUG) {
     pi=(profiler*)malloc((unittab_ptr->ut_nlines+1)*sizeof(profiler));
     if (pi == NULL)
        giveup(SETL_SYSTEM msg_malloc_error);

     unittab_ptr->ut_prof_table=pi;
     for (line=0;line<=unittab_ptr->ut_nlines;line++) {
        pi->count=0;
        pi->copies=0;
#ifdef HAVE_GETRUSAGE
        pi->time.tv_sec=0;
        pi->time.tv_usec=0;
        pi->timec.tv_sec=0;
        pi->timec.tv_usec=0;
#endif
        pi++; 
     }
   }
#endif

   /* load literals */

   load_specifiers(SETL_SYSTEM unittab_ptr,libunit_ptr,&unit_control);
   load_public(SETL_SYSTEM unittab_ptr, libunit_ptr);

   /* execute and free the initialization code */

   save_pc = pc;
   save_ip = ip;
   execute_setup(SETL_SYSTEM unittab_ptr,EX_INIT_CODE);
   critical_section++;
   execute_go(SETL_SYSTEM YES);
   critical_section--;
   ip = save_ip;
   pc = save_pc;

   free(unittab_ptr->ut_init_code);
   unittab_ptr->ut_init_code = NULL;

   /* Update the error_extension map */

   unittab_ptr->ut_err_ext_map = (specifier *)malloc(sizeof(specifier));
   if (unittab_ptr->ut_err_ext_map == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   memcpy(unittab_ptr->ut_err_ext_map,&SYMBOL_MAP,sizeof(specifier));
   SYMBOL_MAP.sp_form = ft_omega;

   /* we're finished with the library */

   free(slot_ptr_tab);
   close_libunit(SETL_SYSTEM libunit_ptr);

   return unittab_ptr;

}

/*\
 *  \function{load\_specifiers()}
\*/

static void load_specifiers(
   SETL_SYSTEM_PROTO
   unittab_ptr_type unittab_ptr,       /* unit to be loaded                 */
   libunit_ptr_type libunit_ptr,       /* read from this unit               */
   unit_control_record *unit_control)  /* unit control record               */

{
#ifdef TSAFE
#define LABEL plugin_instance->label
#else
#define LABEL label
static label_record label;             /* procedure address record  */
#endif

libstr_ptr_type libstr_ptr;            /* work library stream               */
specifier *s;                          /* work specifier pointer            */
integer_record integer;                /* integer literal record            */
real_record real;                      /* real literal record               */
string_record string;                  /* string literal record             */
proc_record proc;                      /* procedure record                  */
integer_h_ptr_type i_hdr;              /* integer header record             */
integer_c_ptr_type i1,i2;              /* integer cell pointers             */
string_h_ptr_type s_hdr;               /* string header record              */
string_c_ptr_type s1,s2;               /* string cell pointers              */
int i;                                 /* temporary looping variable        */
public_record pub;                     /* public symbol record              */
char *symbol;
void *symbol_ptr;

#ifdef WIN32
void *psymb;
#endif

#if HAVE_DLFCN_H
void *phandle;
void *psymb;
#endif

#ifdef MACINTOSH
OSErr myErr;
FSSpec myFSSpec;
Ptr myMainAddr;
Str255 myErrName;
Str63 fragName;
Str255 myName;
long myCount;
CFragSymbolClass myClass;
Ptr myAddr;
#endif


   /* load the integer literals */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INTEGER_STREAM);

   for (i = 0; i < unit_control->uc_integer_count; i++) {

      /* read and build the header */

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&integer,sizeof(integer_record));
      s = unittab_ptr->ut_data_ptr + integer.ir_offset;

      /* load short value (generally) */

      if (integer.ir_cell_count == 1) {

         s->sp_form = ft_short;
         read_libstr(SETL_SYSTEM libstr_ptr,
                     (char *)&(s->sp_val.sp_short_value),
                     sizeof(int32));

      }
      else {

         s->sp_form = ft_long;
         get_integer_header(i_hdr);
         i_hdr->i_use_count = 1;
         i_hdr->i_hash_code = -1;
         i_hdr->i_cell_count = integer.ir_cell_count;
         i_hdr->i_is_negative = NO;

         /* build up the cell list */

         i2 = NULL;
         while (integer.ir_cell_count--) {

            get_integer_cell(i1);
            read_libstr(SETL_SYSTEM libstr_ptr,
                        (char *)&(i1->i_cell_value),
                        sizeof(int32));
            if (i2 == NULL)
               i_hdr->i_head = i1;
            else
               i2->i_next = i1;
            i1->i_prev = i2;
            i2 = i1;

         }

         i1->i_next = NULL;
         i_hdr->i_tail = i1;
         s->sp_val.sp_long_ptr = i_hdr;

      }
   }

   close_libstr(SETL_SYSTEM libstr_ptr);

   /* load the real literals */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_REAL_STREAM);

   for (i = 0; i < unit_control->uc_real_count; i++) {

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&real,sizeof(real_record));
      s = unittab_ptr->ut_data_ptr + real.rr_offset;
      s->sp_form = ft_real;
      i_get_real(s->sp_val.sp_real_ptr);
      (s->sp_val.sp_real_ptr)->r_value = real.rr_value;
      (s->sp_val.sp_real_ptr)->r_use_count = 1;

   }

   close_libstr(SETL_SYSTEM libstr_ptr);

   /* load the string literals */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_STRING_STREAM);

   for (i = 0; i < unit_control->uc_string_count; i++) {

      /* read and build the header record */

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&string,sizeof(string_record));
      s = unittab_ptr->ut_data_ptr + string.sr_offset;
      s->sp_form = ft_string;
      get_string_header(s_hdr);
      s_hdr->s_use_count = 1;
      s_hdr->s_hash_code = -1;
      s_hdr->s_length = string.sr_length;

      /* build up the cell list */

      if (string.sr_length == 0) {

         s_hdr->s_head = s_hdr->s_tail = NULL;

      }
      else {

         s2 = NULL;
         while (string.sr_length > 0) {

            get_string_cell(s1);
            read_libstr(SETL_SYSTEM libstr_ptr,
                        s1->s_cell_value,
                        min(string.sr_length,STR_CELL_WIDTH));
            if (s2 == NULL)
               s_hdr->s_head = s1;
            else
               s2->s_next = s1;
            s1->s_prev = s2;
            s2 = s1;

            string.sr_length -= STR_CELL_WIDTH;

         }

         s1->s_next = NULL;
         s_hdr->s_tail = s1;

      }

      s->sp_val.sp_string_ptr = s_hdr;

   }

   close_libstr(SETL_SYSTEM libstr_ptr);

   /* load the procedures */
   
   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_PROCEDURE_STREAM);

   for (i = 0; i < unit_control->uc_proc_count; i++) {

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&proc,sizeof(proc_record));

#ifdef DEBUG

      if (EX_DEBUG)
         proc.pr_proc_offset *= 2;

#endif

      s = unittab_ptr->ut_data_ptr + proc.pr_symtab_offset;
      (s)->sp_form = ft_proc;
      get_proc(s->sp_val.sp_proc_ptr);
      if (unit_control->uc_type==NATIVE_UNIT) {
         (s->sp_val.sp_proc_ptr)->p_func_ptr = NULL;
         (s->sp_val.sp_proc_ptr)->p_type = NATIVE_PROC;
      } else
         (s->sp_val.sp_proc_ptr)->p_type = USERDEF_PROC;

      (s->sp_val.sp_proc_ptr)->p_signature = s->sp_val.sp_proc_ptr;
      (s->sp_val.sp_proc_ptr)->p_unittab_ptr = unittab_ptr;
      (s->sp_val.sp_proc_ptr)->p_offset = proc.pr_proc_offset;
      (s->sp_val.sp_proc_ptr)->p_formal_count = proc.pr_formal_count;
      (s->sp_val.sp_proc_ptr)->p_spec_count = proc.pr_spec_count;
      (s->sp_val.sp_proc_ptr)->p_spec_ptr =
         unittab_ptr->ut_data_ptr+proc.pr_spec_offset;
      (s->sp_val.sp_proc_ptr)->p_use_count = 1;
      (s->sp_val.sp_proc_ptr)->p_is_const = YES;
      (s->sp_val.sp_proc_ptr)->p_active_use_count = 0;
      (s->sp_val.sp_proc_ptr)->p_copy = NULL;
      (s->sp_val.sp_proc_ptr)->p_save_specs = NULL;
      (s->sp_val.sp_proc_ptr)->p_self_ptr = NULL;
      if (proc.pr_parent_offset != -1) {

         (s->sp_val.sp_proc_ptr)->p_parent =
            (unittab_ptr->ut_data_ptr+proc.pr_parent_offset)->
               sp_val.sp_proc_ptr;
         ((s->sp_val.sp_proc_ptr)->p_parent)->p_use_count++;

      }
      else {

         (s->sp_val.sp_proc_ptr)->p_parent = NULL;

      }
   }

   close_libstr(SETL_SYSTEM libstr_ptr);

   /* Resolve the addresses for procedures in native packages */

   if (unit_control->uc_type==NATIVE_UNIT) {

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr, LIB_PUBLIC_STREAM);

      i = 0;
      symbol = NULL;

      for (;;) {

         if (read_libstr(SETL_SYSTEM libstr_ptr, (char *)&pub, sizeof(public_record)) == 0)
            break;

         s = unittab_ptr->ut_data_ptr + pub.pu_offset;
         assert((s->sp_val.sp_proc_ptr)->p_type == NATIVE_PROC);

         if (pub.pu_name_length>i) {
            i = pub.pu_name_length;
            if (symbol!=NULL) free(symbol);
            symbol = (char *)malloc(i+2);
            if (symbol == NULL)
               giveup(SETL_SYSTEM msg_malloc_error);
         }

         read_libstr(SETL_SYSTEM libstr_ptr, symbol, pub.pu_name_length);

         symbol[pub.pu_name_length]=0;
	
         symbol_ptr=NULL;
#ifdef MACINTOSH
         myErr = FindSymbol((CFragConnectionID)unittab_ptr->ut_native_code,
                            c2pstr(symbol),
                            &myAddr, &myClass);
         p2cstr(symbol);
	   
         if (!myErr)
            symbol_ptr=(void *)myAddr;
#endif

#if HAVE_DLFCN_H
         psymb=dlsym(unittab_ptr->ut_native_code,symbol);
         if (psymb!=NULL)
            symbol_ptr=psymb;
#endif

#ifdef WIN32
         psymb = GetProcAddress((HINSTANCE)(unittab_ptr->ut_native_code), symbol);
         if (psymb!=NULL)
            symbol_ptr=psymb;
#endif

         if (symbol_ptr == NULL)
            giveup(SETL_SYSTEM msg_symbol_not_resolved,symbol);
         else 
            (s->sp_val.sp_proc_ptr)->p_func_ptr = symbol_ptr;

      } 

      close_libstr(SETL_SYSTEM libstr_ptr);

   }

   /* load the label literals */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_LABEL_STREAM);

   for (i = 0; i < unit_control->uc_label_count; i++) {

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&LABEL,sizeof(label_record));

      s = unittab_ptr->ut_data_ptr + LABEL.lr_symtab_offset;
      s->sp_form = ft_label;

#ifdef DEBUG

      if (EX_DEBUG) {
         LABEL.lr_label_offset *= 2;
         if (LABEL.lr_label_offset < 0) 
            LABEL.lr_label_offset += 1;
      }

#endif

      if (LABEL.lr_label_offset < 0) {

         LABEL.lr_label_offset = -LABEL.lr_label_offset - 1;
         s->sp_val.sp_label_ptr = unittab_ptr->ut_init_code +
                                  LABEL.lr_label_offset;

      }
      else {

         s->sp_val.sp_label_ptr = unittab_ptr->ut_body_code +
                                  LABEL.lr_label_offset;

      }
   }

   close_libstr(SETL_SYSTEM libstr_ptr);

}

/*\
 *  \function{load\_public()}
 *
 *  This function loads the public symbols for a package into a map.  We
 *  store the map on the unit table record, and return it if later
 *  requested.
\*/

static void load_public(
   SETL_SYSTEM_PROTO
   unittab_ptr_type unittab_ptr,       /* unit to be loaded                 */
   libunit_ptr_type libunit_ptr)       /* read from this unit               */

{
map_h_ptr_type symbol_map;             /* map from symbol to procedure      */
libstr_ptr_type libstr_ptr;            /* work library stream               */
public_record pub;                     /* public symbol record              */
struct specifier_item key_spec;        /* specifier for map key             */
string_h_ptr_type target_hdr;          /* target string                     */
string_c_ptr_type target_cell;         /* target string cell                */
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
map_c_ptr_type *map_tail;              /* attach new cells here             */
int map_height, map_index;             /* current height and index          */
map_h_ptr_type new_hdr;                /* created header node               */
map_c_ptr_type new_cell;               /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int32 i, j;

   /*
    *  Create an empty map from symbol to procedure pointer.
    */

   get_map_header(symbol_map);
   symbol_map->m_use_count = 1;
   symbol_map->m_hash_code = 0;
   symbol_map->m_ntype.m_root.m_cardinality = 0;
   symbol_map->m_ntype.m_root.m_cell_count = 0;
   symbol_map->m_ntype.m_root.m_height = 0;
   for (i = 0;
        i < MAP_HASH_SIZE;
        symbol_map->m_child[i++].m_cell = NULL);

   /*
    *  Read through the list of public symbols.
    */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr, LIB_PUBLIC_STREAM);
   for (;;) {

      if (read_libstr(SETL_SYSTEM libstr_ptr, (char *)&pub, sizeof(public_record)) == 0)
         break;

      /*
       *  Make a setl2 string out of the procedure name.
       */

      get_string_header(target_hdr);
      target_hdr->s_use_count = 1;
      target_hdr->s_hash_code = -1;
      target_hdr->s_length = pub.pu_name_length;
      target_hdr->s_head = target_hdr->s_tail = NULL;

      /* copy the argument to the string */

      while (pub.pu_name_length > 0) {

         get_string_cell(target_cell);
         if (target_hdr->s_tail != NULL)
            (target_hdr->s_tail)->s_next = target_cell;
         target_cell->s_prev = target_hdr->s_tail;
         target_cell->s_next = NULL;
         target_hdr->s_tail = target_cell;
         if (target_hdr->s_head == NULL)
            target_hdr->s_head = target_cell;

         j = min(pub.pu_name_length, STR_CELL_WIDTH);
         read_libstr(SETL_SYSTEM libstr_ptr, target_cell->s_cell_value, j);
         pub.pu_name_length -= j;

      }

      key_spec.sp_form = ft_string;
      key_spec.sp_val.sp_string_ptr = target_hdr;

      /*
       *  The string is the key into the map.
       */

      map_work_hdr = symbol_map;
      spec_hash_code(work_hash_code, &key_spec);
      for (map_height = symbol_map->m_ntype.m_root.m_height;
           map_height;
           map_height--) {

         /* extract the element's index at this level */

         map_index = work_hash_code & MAP_HASH_MASK;
         work_hash_code >>= MAP_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (map_work_hdr->m_child[map_index].m_header == NULL) {

            get_map_header(new_hdr);
            new_hdr->m_ntype.m_intern.m_parent = map_work_hdr;
            new_hdr->m_ntype.m_intern.m_child_index = map_index;
            for (i = 0;
                 i < MAP_HASH_SIZE;
                 new_hdr->m_child[i++].m_cell = NULL);
            map_work_hdr->m_child[map_index].m_header = new_hdr;
            map_work_hdr = new_hdr;

         }
         else {

            map_work_hdr = map_work_hdr->m_child[map_index].m_header;

         }
      }

      /*
       *  At this point, map_work_hdr points to the lowest level
       *  header record.
       */

      map_index = work_hash_code & MAP_HASH_MASK;
      map_tail = &(map_work_hdr->m_child[map_index].m_cell);
      spec_hash_code(work_hash_code, &key_spec);
      for (map_cell = *map_tail;
           map_cell != NULL &&
              map_cell->m_hash_code < work_hash_code;
           map_cell = map_cell->m_next) {

         map_tail = &(map_cell->m_next);

      }

      /* we don't have to worry about duplicates -- add a cell */

      get_map_cell(new_cell);
      new_cell->m_domain_spec.sp_form = ft_string;
      new_cell->m_domain_spec.sp_val.sp_string_ptr = target_hdr;
      memcpy(&(new_cell->m_range_spec),
             unittab_ptr->ut_data_ptr + pub.pu_offset,
             sizeof(struct specifier_item));
      mark_specifier(&(new_cell->m_range_spec));
      new_cell->m_is_multi_val = NO;
      spec_hash_code(new_cell->m_hash_code, &(new_cell->m_domain_spec));
      new_cell->m_next = *map_tail;
      *map_tail = new_cell;
      symbol_map->m_ntype.m_root.m_cardinality++;
      symbol_map->m_ntype.m_root.m_cell_count++;
      symbol_map->m_hash_code ^= new_cell->m_hash_code;

      expansion_trigger =
            (1 << ((symbol_map->m_ntype.m_root.m_height + 1)
                       * MAP_SHIFT_DIST)) * MAP_CLASH_SIZE;

      /* expand the map header if necessary */

      if (symbol_map->m_ntype.m_root.m_cardinality > expansion_trigger) {
         symbol_map = map_expand_header(SETL_SYSTEM symbol_map);
      }

   }

   close_libstr(SETL_SYSTEM libstr_ptr);
   unittab_ptr->ut_symbol_map = symbol_map;

}

/*\
 *  \function{load\_pcode()}
 *
 *  This function loads a stream of pseudo code from a library.  We
 *  require an open stream and a count of the pseudo code instructions to
 *  be loaded from the caller.
\*/

static instruction *load_pcode(
   SETL_SYSTEM_PROTO
   libstr_ptr_type libstr_ptr,         /* read pcode from this stream       */
   unittab_ptr_type unittab_ptr,       /* unit to be loaded                 */
   slot_ptr_type *slot_ptr_tab,        /* convert local to global slots     */
   int32 pcode_count)                  /* number of pcode instructions      */
                                       /* locations                         */

{
instruction *return_ptr;               /* returned instruction list         */
instruction *p;                        /* next instruction pointer          */
pcode_record pcode;                    /* pseudo code record                */
int operand;                           /* used to loop over operands        */
int i;                                 /* temporary looping variable        */
long nlines;                           /* number of lines                   */

   /* allocate space for the pseudo code instructions */

#ifdef DEBUG
	
   return_ptr = (instruction *)malloc(
      (size_t)((pcode_count + 1) * sizeof(instruction) * (1+EX_DEBUG)));

#else

   return_ptr = (instruction *)malloc(
      (size_t)((pcode_count + 1) * sizeof(instruction)));

#endif

   if (return_ptr == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   p = return_ptr;

   nlines = 0;

   /* loop through the instructions */

   for (i = 0; i < pcode_count; i++) {

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&pcode,sizeof(pcode_record));

#ifdef DEBUG

      if (EX_DEBUG) {

         p->i_opcode = p_filepos;
         p->i_operand[0].i_class_ptr = unittab_ptr;
         p->i_operand[1].i_integer = pcode.pr_file_pos.fp_line;
         p->i_operand[2].i_integer = pcode.pr_file_pos.fp_column;
         p++;
         if (pcode.pr_file_pos.fp_line>nlines) 
            nlines=pcode.pr_file_pos.fp_line;

     }

#endif

      p->i_opcode = pcode.pr_opcode;

      for (operand = 0; operand < 3; operand++) {

         switch (pcode_optype[pcode.pr_opcode][operand]) {

            case PCODE_INTEGER_OP :

               p->i_operand[operand].i_integer = pcode.pr_offset[operand];

               break;

            case PCODE_SPEC_OP :

               if (pcode.pr_unit_num[operand] != -1)
                  p->i_operand[operand].i_spec_ptr =
                     (unittab_ptr->ut_unit_tab[pcode.pr_unit_num[operand]])->
                        ut_data_ptr +
                     pcode.pr_offset[operand];
               else
                  p->i_operand[operand].i_spec_ptr = NULL;

               break;

            case PCODE_INST_OP :

               if (pcode.pr_unit_num[operand] != -1) {

#ifdef DEBUG

                  if (EX_DEBUG) {
                     pcode.pr_offset[operand] *= 2;
                     if (pcode.pr_offset[operand] < 0) 
                        pcode.pr_offset[operand] += 1;
                  }

#endif

                  if (pcode.pr_offset[operand] < 0) 
                     pcode.pr_offset[operand] = -pcode.pr_offset[operand] - 1;

                  p->i_operand[operand].i_inst_ptr =
                     return_ptr + pcode.pr_offset[operand];

               }
               else
                  p->i_operand[operand].i_inst_ptr = NULL;

               break;

            case PCODE_SLOT_OP :

               p->i_operand[operand].i_slot =
                  slot_ptr_tab[pcode.pr_offset[operand]]->sl_number;

               break;

            case PCODE_CLASS_OP :

               p->i_operand[operand].i_class_ptr =
                  (unittab_ptr->ut_unit_tab[pcode.pr_offset[operand]]);

               break;

         }
      }

      p++;

   }

   /* to be safe, append a stop instruction */

   p->i_opcode = p_stop;
   p->i_operand[0].i_func_ptr = NULL;
   p->i_operand[1].i_func_ptr = NULL;
   p->i_operand[2].i_func_ptr = NULL;

#ifdef DEBUG
   if ((EX_DEBUG)&&(unittab_ptr->ut_nlines<nlines))
     unittab_ptr->ut_nlines=nlines;
#endif

   return return_ptr;

}

#ifdef DYNAMIC_COMP

/*\
 *  \function{load\_eval_unit()}
 *
 *  This function loads a unit from the libraries into the various memory
 *  structures.
\*/

unittab_ptr_type load_eval_unit(
   SETL_SYSTEM_PROTO
   char *unit_name,                    /* name of unit to be loaded         */
   unittab_ptr_type unit_parent,       /* parent pointer                    */
   char *base_name)                    /* base for inherited units          */

{
unittab_ptr_type unittab_ptr;          /* returned unit pointer             */
unittab_ptr_type temp_parent;          /* parent pointer (looping)          */
libunit_ptr_type libunit_ptr;          /* current unit                      */
libstr_ptr_type libstr_ptr;            /* library stream pointer            */
unit_control_record unit_control;      /* unit control record               */
slot_record slot;                      /* slot record                       */
char name_buffer[MAX_TOK_LEN];         /* name buffer for slot              */
slot_ptr_type *slot_ptr_tab, slot_ptr; /* pointer for each slot             */
int32 work_length;                     /* working length (destroyed)        */
import_record import;                  /* imported unit record              */
unittab_ptr_type import_unit;          /* pointer to imported unit          */
int32 i,j;                             /* temporary looping variables       */
struct slot_info_item **next_var;      /* next instance variable            */
int32 slot_index;                      /* slot index within instance        */
instruction *save_pc;                  /* saved instruction pointer         */
instruction *save_ip;                  /* saved instruction pointer         */
int first; /* Flag: True first time an eval unit is loaded */
long line;                             /* temporary looping variable        */
struct profiler_item *pi;              /* temporary looping variable        */

   /*
    *  First open a new unit table record.  We form the unit name by
    *  prepending the base name, then look for the unit in the unit
    *  table.  If we find it already, then return.  Otherwise we have to
    *  load the unit.
    */

   unittab_ptr = get_unittab(SETL_SYSTEM unit_name);

   /* update ancestors' unit tables */

   for (temp_parent = unit_parent;
        temp_parent != NULL;
        temp_parent = temp_parent->ut_parent) {

      temp_parent->ut_unit_tab[temp_parent->ut_units_loaded++] = unittab_ptr;
      if (unittab_ptr->ut_is_loaded) {
         for (i = 2; i <= unittab_ptr->ut_last_inherit; i++) {
            temp_parent->ut_unit_tab[temp_parent->ut_units_loaded++] =
               unittab_ptr->ut_unit_tab[i];
         }
      }
   }

   /* if we were previously loaded, don't load us again */

   if (unittab_ptr->ut_is_loaded)
      first=YES;
   else
      first=NO;

   unittab_ptr->ut_is_loaded = YES;
   unittab_ptr->ut_self = NULL;
   unittab_ptr->ut_err_ext_map=NULL;

   /* find the unit in the library */

   libunit_ptr = open_libunit(SETL_SYSTEM unit_name,NULL,LIB_READ_UNIT);
   if (libunit_ptr == NULL)
      return NULL;

   /* load the unit control record */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_CONTROL_STREAM);
   read_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
               sizeof(unit_control_record));
   close_libstr(SETL_SYSTEM libstr_ptr);

   /* make sure package bodies are compiled */

   if (unit_control.uc_needs_body)
      giveup(SETL_SYSTEM msg_package_uncompiled,unit_name);

   /* start filling in the unit table record */

   unittab_ptr->ut_type = unit_control.uc_type;
   strcpy(unittab_ptr->ut_source_name,unit_control.uc_spec_source_name);
   unittab_ptr->ut_time_stamp = unit_control.uc_time_stamp;

   /* allocate space for specifiers */

/* SPAM */
   unittab_ptr->ut_data_ptr =
      get_specifiers(SETL_SYSTEM unit_control.uc_spec_count + 1);

   /* allocate a unit table */

   unittab_ptr->ut_unit_tab = (unittab_ptr_type *)malloc(
      (size_t)((unit_control.uc_unit_count + 2) *
               sizeof(unittab_ptr_type *)));
   if (unittab_ptr->ut_unit_tab == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   unittab_ptr->ut_unit_tab[0] = get_unittab(SETL_SYSTEM "$predefined");
   unittab_ptr->ut_unit_tab[1] = unittab_ptr;
   unittab_ptr->ut_units_loaded = 2;

   /* load units imported with an 'inherit' clause (propagate back) */

/* 
 * This should not be necessary... 
 *
 * unittab_ptr->ut_parent = unit_parent;
 * libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INHERIT_STREAM);
 *
 * for (i = 0; i < unit_control.uc_inherit_count; i++) {
 *
 *    read_libstr(SETL_SYSTEM libstr_ptr,(char *)&import,sizeof(import_record));
 *    if (base_name != NULL)
 *       import_unit = load_unit(SETL_SYSTEM import.ir_name,unittab_ptr,base_name);
 *    else
 *       import_unit = load_unit(SETL_SYSTEM import.ir_name,unittab_ptr,unit_name);
 *
 *    if (import_unit->ut_type != CLASS_UNIT)
 *       giveup(SETL_SYSTEM "Expected %s to be a class",
 *              import.ir_name);
 *
 *    if (strcmp(import_unit->ut_source_name,import.ir_source_name) != 0 ||
 *        import_unit->ut_time_stamp != import.ir_time_stamp)
 *       giveup(SETL_SYSTEM msg_package_needs_compiled,unit_name);
 *
 * }
 *
 * close_libstr(SETL_SYSTEM libstr_ptr);
 *
 */

   /* load units imported with a 'use' clause (don't propagate)  */

   unittab_ptr->ut_last_inherit = unittab_ptr->ut_units_loaded - 1;
   unittab_ptr->ut_parent = NULL;

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_IMPORT_STREAM);

   for (i = 0; i < unit_control.uc_import_count; i++) {

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&import,sizeof(import_record));
      import_unit = load_unit(SETL_SYSTEM import.ir_name,unittab_ptr,NULL);

      if (import_unit->ut_type == PROGRAM_UNIT)
         giveup(SETL_SYSTEM msg_expected_pack_not_unit,
                import.ir_name);

      if (strcmp(import_unit->ut_source_name,import.ir_source_name) != 0 ||
          import_unit->ut_time_stamp != import.ir_time_stamp)
         giveup(SETL_SYSTEM msg_package_needs_compiled,unit_name);

   }

   close_libstr(SETL_SYSTEM libstr_ptr);
   unittab_ptr->ut_parent = unit_parent;

   /* build the slot table */

   slot_ptr_tab = (slot_ptr_type *)malloc(
      (size_t)((unit_control.uc_max_slot + 2) * sizeof(slot_ptr_type)));

   /*
    *  We make two passes over the slot stream.  The first pass just adds
    *  all the slot names to the slot table.  We have to do this first to
    *  build an array relating internal slot numbers to global slot
    *  numbers.
    */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_DSLOT_STREAM);

   for (i = 0; i < unit_control.uc_slot_count; i++) {

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&slot,sizeof(slot_record));

      read_libstr(SETL_SYSTEM libstr_ptr,
                  name_buffer,
                  (size_t)slot.sl_name_length);

      name_buffer[slot.sl_name_length] = '\0';

      slot_ptr = get_slot(SETL_SYSTEM name_buffer);
      slot_ptr_tab[slot.sl_number] = slot_ptr;

   }

   close_libstr(SETL_SYSTEM libstr_ptr);

   /*
    *  Now we start the second pass.  The goal here is to build up the
    *  slot information array in the unit table.  We only have to do this
    *  for classes imported with a 'use' clause.
    */

   if ((unittab_ptr->ut_type == CLASS_UNIT ||
        unittab_ptr->ut_type == PROCESS_UNIT) &&
       base_name == NULL) {

      /* build the slot table */

      unittab_ptr->ut_slot_count = TOTAL_SLOT_COUNT;
      unittab_ptr->ut_slot_info = (struct slot_info_item *)malloc(
         (size_t)(TOTAL_SLOT_COUNT * sizeof(struct slot_info_item)));
      unittab_ptr->ut_first_var = NULL;
      next_var = &(unittab_ptr->ut_first_var);
      unittab_ptr->ut_var_count = 0;
      slot_index = 0;

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_DSLOT_STREAM);

      for (j = 0; j < TOTAL_SLOT_COUNT; j++)
         unittab_ptr->ut_slot_info[j].si_in_class = NO;

      for (i = 0; i < unit_control.uc_slot_count; i++) {

         read_libstr(SETL_SYSTEM libstr_ptr,(char *)&slot,sizeof(slot_record));

         read_libstr(SETL_SYSTEM libstr_ptr,
                     name_buffer,
                     (size_t)slot.sl_name_length);

         name_buffer[slot.sl_name_length] = '\0';

         j = slot_ptr_tab[slot.sl_number]->sl_number;

         /*
          *  MPW C doesn't seem to be able to copy bit fields.  I hope I
          *  found all the places I do that!
          */

#ifdef MPWC

{
int mpw_bug_workaround;

         mpw_bug_workaround = slot.sl_is_method;
         unittab_ptr->ut_slot_info[j].si_is_method = mpw_bug_workaround;
         mpw_bug_workaround = slot.sl_is_public;
         unittab_ptr->ut_slot_info[j].si_is_public = mpw_bug_workaround;
         mpw_bug_workaround = slot.sl_in_class;
         unittab_ptr->ut_slot_info[j].si_in_class = mpw_bug_workaround;

}
#else

         unittab_ptr->ut_slot_info[j].si_is_method = slot.sl_is_method;
         unittab_ptr->ut_slot_info[j].si_is_public = slot.sl_is_public;
         unittab_ptr->ut_slot_info[j].si_in_class = slot.sl_in_class;

#endif

         unittab_ptr->ut_slot_info[j].si_slot_ptr =
            slot_ptr_tab[slot.sl_number];
         unittab_ptr->ut_slot_info[j].si_spec =
            (unittab_ptr->ut_unit_tab[slot.sl_unit_num])->ut_data_ptr +
               slot.sl_offset;
         if (slot.sl_in_class && !slot.sl_is_method) {
            unittab_ptr->ut_slot_info[j].si_index = slot_index++;
            *next_var = unittab_ptr->ut_slot_info + j;
            next_var = &(unittab_ptr->ut_slot_info[j].si_next_var);
            *next_var = NULL;
            unittab_ptr->ut_var_count++;
         }
         else {
            unittab_ptr->ut_slot_info[j].si_index = -1;
         }
      }

      /* calculate height of header tree */

      work_length = unittab_ptr->ut_var_count;
      unittab_ptr->ut_obj_height = 0;
      while (work_length = work_length >> OBJ_SHIFT_DIST)
         unittab_ptr->ut_obj_height++;

      close_libstr(SETL_SYSTEM libstr_ptr);

   }

   /* load the initialization code */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INIT_STREAM);
   unittab_ptr->ut_init_code =
      load_pcode(SETL_SYSTEM libstr_ptr,
                 unittab_ptr,
                 slot_ptr_tab,
                 unit_control.uc_ipcode_count);
   close_libstr(SETL_SYSTEM libstr_ptr);

   /* load the body code */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_PCODE_STREAM);
   unittab_ptr->ut_body_code =
      load_pcode(SETL_SYSTEM libstr_ptr,
                 unittab_ptr,
                 slot_ptr_tab,
                 unit_control.uc_bpcode_count);
   close_libstr(SETL_SYSTEM libstr_ptr);

#ifdef DEBUG
   if (PROF_DEBUG) {
     pi=(profiler*)malloc((unittab_ptr->ut_nlines+1)*sizeof(profiler));
     if (pi == NULL)
        giveup(SETL_SYSTEM msg_malloc_error);

     unittab_ptr->ut_prof_table=pi;
     for (line=0;line<=unittab_ptr->ut_nlines;line++) {
        pi->count=0;
        pi->copies=0;
#ifdef HAVE_GETRUSAGE
        pi->time.tv_sec=0;
        pi->time.tv_usec=0;
        pi->timec.tv_sec=0;
        pi->timec.tv_usec=0;
#endif
        pi++; 
     }
   }
#endif

   /* load literals */

   load_specifiers(SETL_SYSTEM unittab_ptr,libunit_ptr,&unit_control);
   load_public(SETL_SYSTEM unittab_ptr, libunit_ptr);

   /* execute and free the initialization code */

   save_pc = pc;
   save_ip = ip;
   execute_setup(SETL_SYSTEM unittab_ptr,EX_INIT_CODE);
   critical_section++;
   execute_go(SETL_SYSTEM YES);
   critical_section--;
   ip = save_ip;
   pc = save_pc;

   free(unittab_ptr->ut_init_code);
   unittab_ptr->ut_init_code = NULL;

   /* Update the error_extension map */

   unittab_ptr->ut_err_ext_map = (specifier *)malloc(sizeof(specifier));
   if (unittab_ptr->ut_err_ext_map == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   memcpy(unittab_ptr->ut_err_ext_map,&SYMBOL_MAP,sizeof(specifier));
   SYMBOL_MAP.sp_form = ft_omega;

   /* we're finished with the library */

   free(slot_ptr_tab);
   close_libunit(SETL_SYSTEM libunit_ptr);

   return unittab_ptr;

}

#endif /* DYNAMIC_COMP */
