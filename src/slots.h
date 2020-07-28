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
 *  \packagespec{Slot Table}
\*/

#ifndef SLOTS_LOADED

/* slot table item structure */

struct slot_item {
   int sl_type;                        /* slot type                         */
   int32 sl_number;                    /* global slot number                */
   struct slot_item *sl_hash_link;     /* next slot with same hash value    */
   char *sl_name;                      /* slot lexeme                       */
};

typedef struct slot_item *slot_ptr_type;
                                       /* slot table type definition        */

/* clear a slot item */

#define clear_slot(s) { \
   (s)->sl_type = -1;                  (s)->sl_hash_link = NULL; \
   (s)->sl_name = NULL;                (s)->sl_number = 0; \
}

#ifdef TSAFE
#define TOTAL_SLOT_COUNT plugin_instance->total_slot_count 
#else
#define TOTAL_SLOT_COUNT total_slot_count 
#ifdef SHARED

int32 total_slot_count;                /* total number of allocated slots   */

#else

extern int32 total_slot_count;         /* total number of allocated slots   */

#endif
#endif

/* public function declarations */

void init_slots(SETL_SYSTEM_PROTO_VOID); 
                                       /* initialize slot table             */
slot_ptr_type get_slot(SETL_SYSTEM_PROTO char *);    
                                       /* get slot table item               */


/* performance tuning constants */

#define SLOTS__HASH_TABLE_SIZE    13       /* size of hash table                */

#define SLOTS_LOADED
#endif
