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
 *
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "cmdline.h"                   /* command line argument string      */
#include "form.h"                      /* form codes                        */
#include "filename.h"                  /* file name utilities               */
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
#include "tuples.h"                    /* tuples                            */
#include "slots.h"                     /* procedures                        */
#include "pcode.h"                     /* pseudo code                       */
#include "shared.h"

#ifdef PLUGIN
#define printf plugin_printf
#endif

extern int abend_initialized;



#ifdef WIN32
#include <windows.h>
#include "axobj.h"
static void ax_destructor(struct setl_ax *spec)
{


}

#endif

#ifdef TSAFE
plugin_item_ptr_type Setl_Initialize(
)
{
struct plugin_item *plugin_instance;

   plugin_instance=(struct plugin_item *)malloc(sizeof(struct plugin_item));

   if (plugin_instance==NULL) return NULL;
   bzero(plugin_instance, sizeof(struct plugin_item));
			       
   plugin_instance->file_next_free=NULL;
   plugin_instance->integer_h_next_free=NULL;
   plugin_instance->integer_c_next_free=NULL;
   plugin_instance->mailbox_h_next_free=NULL;
   plugin_instance->mailbox_c_next_free=NULL;
   plugin_instance->process_next_free=NULL;
   plugin_instance->request_next_free=NULL;
   plugin_instance->string_h_next_free=NULL;
   plugin_instance->string_c_next_free=NULL;
   plugin_instance->iter_next_free=NULL;
   plugin_instance->set_h_next_free=NULL;
   plugin_instance->set_c_next_free=NULL;
   plugin_instance->map_h_next_free=NULL;
   plugin_instance->map_c_next_free=NULL;
   plugin_instance->tuple_h_next_free=NULL;
   plugin_instance->tuple_c_next_free=NULL;
   plugin_instance->real_next_free=NULL;
   plugin_instance->proc_next_free=NULL;
   plugin_instance->object_h_next_free=NULL;
   plugin_instance->object_c_next_free=NULL;
   plugin_instance->self_stack_next_free=NULL;
   plugin_instance->debug_file=stdout;
   plugin_instance->assert_mode=0;
   plugin_instance->ex_debug=0;
   plugin_instance->alloc_debug=0;
   plugin_instance->prof_debug=0;
   plugin_instance->copy_debug=0;
   plugin_instance->step_debug=0;
   plugin_instance->tracing_on=0;
   plugin_instance->trace_copies=0;
   plugin_instance->opcode_executed=0;
   plugin_instance->markup_source=0;
   plugin_instance->default_library="setl2.lib";
   plugin_instance->library_path="";
   plugin_instance->opcode_count=0;
   plugin_instance->process_slice=2000;
   plugin_instance->nested_calls=0;
   plugin_instance->eval_package=0;
   plugin_instance->verbose_mode=0;
   plugin_instance->wait_flag=0;
   plugin_instance->total_slot_count=0;

   plugin_instance->already_called = NO;
   plugin_instance->first_time = YES;
   plugin_instance->arg_buffer = NULL;
   plugin_instance->carg_num = 0;
   plugin_instance->table_block_head = NULL;
   plugin_instance->table_next_free = NULL;
   plugin_instance->string_block_head = NULL;
   plugin_instance->string_block_eos = NULL;
   plugin_instance->string_next_free = NULL;
   plugin_instance->unittab__table_block_head = NULL;
   plugin_instance->unittab__table_next_free = NULL;
   plugin_instance->unittab__string_block_head = NULL;
   plugin_instance->unittab__string_block_eos = NULL;
   plugin_instance->unittab__string_next_free = NULL;
   plugin_instance->reg_types=NULL;
   plugin_instance->num_reg_types=0;
   plugin_instance->x_source_name[0]=0;
   plugin_instance->safe_mode=0;
   plugin_instance->safe_prefix=NULL;
	
   
#else 

void Setl_Initialize() 
{ 
#ifdef DYNAMIC_COMP
   compiler_init();
#endif
#endif

   DEBUG_FILE=stdout;
   EVAL_PACKAGE=NO;  
   REG_TYPES=NULL;
   NUM_REG_TYPES=0;
   abend_initialized=0;
   strcpy(setl2_shlib_path,"");
      
#ifdef TSAFE
   return plugin_instance;
#endif

}

void Setl_SetVerboseMode(
   SETL_SYSTEM_PROTO
   int mode) 
{ 

   VERBOSE_MODE=mode;
}

void Setl_InitInterpreter(
   SETL_SYSTEM_PROTO_VOID
   )
{ 
int i;

#ifdef DEBUG

   if (PROF_DEBUG) {
       for (i=0;i<=pcode_length;i++) {
          pcode_operations[i]=0;
          copy_operations[i]=0;
       }
       head_unittab=NULL; 
       last_unittab=NULL; 
   }
#endif

   init_interp_reals(SETL_SYSTEM_VOID);
   init_unittab(SETL_SYSTEM_VOID);
   init_slots(SETL_SYSTEM_VOID);
   open_io(SETL_SYSTEM_VOID);
   open_lib();
}


void Setl_PrintVersion(
   SETL_SYSTEM_PROTO_VOID
   )
{ 
         plugin_printf("SETL2 System Version %s (%s)\n",VERSION, PLATFORM);
         printf("by W. Kirk Snyder, Salvatore Paxia, Jack Schwartz, Giuseppe Di Mauro \n\n");

}


