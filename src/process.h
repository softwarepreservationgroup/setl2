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
 *  \packagespec{Processes}
\*/

#ifndef PROCESS_LOADED

/* process node structure */

#define ROOT_PROCESS    0              /* main program                      */
#define CHILD_PROCESS   1              /* generated processes               */

/* process node structure */

struct process_item {
   unsigned pc_type : 1;               /* root or childe                    */
   unsigned pc_idle : 1;               /* YES if not working on a request   */
   unsigned pc_suspended : 1;          /* YES if manually suspended         */
   unsigned pc_waiting : 1;            /* YES if waiting for mailbox        */
   unsigned pc_checking : 1;           /* YES if checking mailbox           */
   struct specifier_item pc_wait_key;  /* key to await or acheck            */
   struct specifier_item pc_wait_return; 
                                       /* return value from wait            */
   struct specifier_item *pc_wait_target; 
                                       /* target for return from wait       */
   struct process_item *pc_prev;       /* previous process in pool          */
   struct process_item *pc_next;       /* next value in process pool        */
   struct object_h_item *pc_object_ptr;
                                       /* object record for process         */
   struct request_item *pc_request_head;
                                       /* first pending request             */
   struct request_item **pc_request_tail;
                                       /* next request should be placed     */
                                       /* here                              */

   /* saved stacks */

   instruction *pc_pc;
   instruction *pc_ip;
   int32 pc_pstack_base;
   int32 pc_pstack_top;
   int32 pc_pstack_max;
   struct specifier_item *pc_pstack;
   int32 pc_cstack_top;
   int32 pc_cstack_max;
   struct call_stack_item *pc_cstack;
   struct unittab_item *pc_current_class;
};

typedef struct process_item *process_ptr_type;
                                       /* process item type                 */

/* request node structure */

struct request_item {
   struct request_item *rq_next;       /* next pending request              */
   struct mailbox_h_item *rq_mailbox_ptr;
                                       /* mailbox for reply                 */
   struct proc_item *rq_proc;          /* entry procedure                   */
   struct specifier_item *rq_args;     /* arguments for request             */
};

typedef struct request_item *request_ptr_type;
                                       /* request item type                 */
#ifdef TSAFE
#define PROCESS_NEXT_FREE plugin_instance->process_next_free 
#define REQUEST_NEXT_FREE plugin_instance->request_next_free 
#else
#define PROCESS_NEXT_FREE process_next_free 
#define REQUEST_NEXT_FREE request_next_free 

#ifdef SHARED
        
process_ptr_type process_next_free = NULL;
                                       /* next free process                 */
request_ptr_type request_next_free = NULL;
                                       /* next free request                 */

#else

extern process_ptr_type process_next_free;
                                       /* next free process                 */
extern request_ptr_type request_next_free;
                                       /* next free request                 */

#endif
#endif

/* allocate and free process nodes */

#define get_process(t) {\
   if (PROCESS_NEXT_FREE == NULL) alloc_processes(SETL_SYSTEM_VOID); \
   t = PROCESS_NEXT_FREE; \
   PROCESS_NEXT_FREE = *((process_ptr_type *)(PROCESS_NEXT_FREE)); \
}

#define free_process(s) {\
   *((process_ptr_type *)(s)) = PROCESS_NEXT_FREE; \
   PROCESS_NEXT_FREE = s; \
}

/* allocate and free request nodes */

#define get_request(t) {\
   if (REQUEST_NEXT_FREE == NULL) alloc_requests(SETL_SYSTEM_VOID); \
   t = REQUEST_NEXT_FREE; \
   REQUEST_NEXT_FREE = *((request_ptr_type *)(REQUEST_NEXT_FREE)); \
}

#define free_request(s) {\
   *((request_ptr_type *)(s)) = REQUEST_NEXT_FREE; \
   REQUEST_NEXT_FREE = s; \
}

/* public function declarations */

void alloc_processes(SETL_SYSTEM_PROTO_VOID);            
                                       /* allocate a block of process nodes */
int process_unblock(SETL_SYSTEM_PROTO struct process_item *);
                                       /* try to unblock a waiting process  */
void alloc_requests(SETL_SYSTEM_PROTO_VOID); 
                                       /* allocate a block of request nodes */

#define PROCESS_LOADED 1
#endif


