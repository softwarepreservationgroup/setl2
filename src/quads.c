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
 *  \package{Quadruples}
 *
 *  We use a three address intermediate code in the translation process.
 *  This is very similar to the final code generated, except that the
 *  operands are symbol table pointers rather than memory addresses.
 *  This package contains several low-level functions which manipulate
 *  those quadruples.  The table is dynamically allocated in blocks, like
 *  most of the other tables in the compiler.  The functions which
 *  allocate and free nodes are fairly standard.
 *
 *  We also provide macros which emit quadruples.  We do this here in
 *  order to isolate code which accesses the intermediate code files.  We
 *  prefer to write directly to the intermediate files rather than build
 *  up a list in memory and then write it.  This nearly doubles the size
 *  of a procedure we can handle, since we avoid keeping two intermediate
 *  code forms in memory simultaneously.
 *
 *  \texify{quads.h}
 *
 *  \packagebody{Quadruples}
\*/



/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "quads.h"                     /* quadruples                        */

#ifdef PLUGIN
#define fprintf plugin_fprintf
#define fputs   plugin_fputs
#endif


/* performance tuning constants */

#define QUAD_BLOCK_SIZE      200       /* quad block size                   */

/* generic table item structure (quadruple  use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct quad_item ti_data;        /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[QUAD_BLOCK_SIZE];
                                       /* array of table items              */
};

/* package-global data */

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */
static struct quad_item emit_record;   /* storage for emitted quadruple     */

/*\
 *  \function{init\_quads()}
 *
 *  This procedure initializes the quadruple table.  All we do is free
 *  all the allocated blocks.
\*/

void init_quads()

{
struct table_block *tb_ptr;            /* work table pointer                */

   while (table_block_head != NULL) {
      tb_ptr = table_block_head;
      table_block_head = table_block_head->tb_next;
      free((void *)tb_ptr);
   }
   table_next_free = NULL;

}

/*\
 *  \function{get\_quad()}
 *
 *  This procedure allocates a quadruple node. It is just like most of
 *  the other dynamic table allocation functions in the compiler.
\*/

quad_ptr_type get_quad(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *old_head;          /* quadruple table block list head   */
quad_ptr_type return_ptr;              /* return pointer                    */

   if (table_next_free == NULL) {

      /* allocate a new block */

      old_head = table_block_head;
      table_block_head = (struct table_block *)
                         malloc(sizeof(struct table_block));
      if (table_block_head == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      table_block_head->tb_next = old_head;

      /* link items on the free list */

      for (table_next_free = table_block_head->tb_data;
           table_next_free <=
              table_block_head->tb_data + QUAD_BLOCK_SIZE - 2;
           table_next_free++) {
         table_next_free->ti_union.ti_next = table_next_free + 1;
      }

      table_next_free->ti_union.ti_next = NULL;
      table_next_free = table_block_head->tb_data;
   }

   /* at this point, we know the free list is not empty */

   return_ptr = &(table_next_free->ti_union.ti_data);
   table_next_free = table_next_free->ti_union.ti_next;

   /* initialize the new entry */

   clear_quad(return_ptr);

   return return_ptr;

}

/*\
 *  \function{free\_quad()}
 *
 *  This function is the complement to \verb"get_quad()". All we do is
 *  push the passed quadruple pointer on the free list.
\*/

void free_quad(
   SETL_SYSTEM_PROTO
   quad_ptr_type discard)              /* item to be discarded              */

{

   ((struct table_item *)discard)->ti_union.ti_next = table_next_free;
   table_next_free = (struct table_item *)discard;

}

/*\
 *  \function{open\_emit()}
 *
 *  This function sets up global variables so that subsequent calls to
 *  \verb"emit()" insert quadruples on the current list, or write them to
 *  the intermediate file.
\*/

void open_emit(
   SETL_SYSTEM_PROTO
   storage_location *location)         /* where quads should be placed      */

{

   if (USE_INTERMEDIATE_FILES) {

      /* we'll start writing at the end of file */

      if (fseek(I1_FILE,0L,SEEK_END) != 0)
         giveup(SETL_SYSTEM msg_bad_inter_fseek);
      location->sl_file_ptr = ftell(I1_FILE);
      if (location->sl_file_ptr == -1)
         giveup(SETL_SYSTEM msg_bad_inter_fseek);

      emit_quad_ptr = &emit_record;

   }
   else {

      emit_quad_tail = (void *)(&(location->sl_mem_ptr));

   }
}

/*\
 *  \function{emit()}
 *
 *  We have three forms of the \verb"emit()" function, depending on the
 *  form of their arguments.  We just write a quad to an intermediate
 *  file or a memory list.
\*/

#if SHORT_MACROS

/* general form -- symbol table operands */

void emit(
   int p,                              /* opcode                            */
   symtab_ptr_type o1,                 /* operands                          */
   symtab_ptr_type o2,
   symtab_ptr_type o3,
   file_pos_type *fp)                  /* source file position              */

{

   if (!USE_INTERMEDIATE_FILES) {

      emit_quad_ptr = get_quad(SETL_SYSTEM_VOID);
      *emit_quad_tail = emit_quad_ptr; 
      emit_quad_tail = &(emit_quad_ptr->q_next);

   } 

   emit_quad_ptr->q_opcode = p; 
   emit_quad_ptr->q_operand[0].q_symtab_ptr = (o1); 
   emit_quad_ptr->q_operand[1].q_symtab_ptr = (o2); 
   emit_quad_ptr->q_operand[2].q_symtab_ptr = (o3); 
   emit_quad_ptr->q_file_pos.fp_line = (fp)->fp_line; 
   emit_quad_ptr->q_file_pos.fp_column = (fp)->fp_column; 

   if (USE_INTERMEDIATE_FILES)  
      if (fwrite((void *)emit_quad_ptr,sizeof(struct quad_item), 
                 (size_t)1,I1_FILE) != (size_t)1) 
         giveup(SETL_SYSTEM msg_iter_write_error); 

}

/* first argument integer, others symbol table pointers */

void emitiss(
   int p,                              /* opcode                            */
   int o1,                             /* operands                          */
   symtab_ptr_type o2,
   symtab_ptr_type o3,
   file_pos_type *fp)                  /* source file position              */

{

   if (!USE_INTERMEDIATE_FILES) {

      emit_quad_ptr = get_quad(SETL_SYSTEM_VOID);
      *emit_quad_tail = emit_quad_ptr; 
      emit_quad_tail = &(emit_quad_ptr->q_next);

   } 

   emit_quad_ptr->q_opcode = p; 
   emit_quad_ptr->q_operand[0].q_integer = (o1); 
   emit_quad_ptr->q_operand[1].q_symtab_ptr = (o2); 
   emit_quad_ptr->q_operand[2].q_symtab_ptr = (o3); 
   emit_quad_ptr->q_file_pos.fp_line = (fp)->fp_line; 
   emit_quad_ptr->q_file_pos.fp_column = (fp)->fp_column; 

   if (USE_INTERMEDIATE_FILES)  
      if (fwrite((void *)emit_quad_ptr,sizeof(struct quad_item), 
                 (size_t)1,I1_FILE) != (size_t)1) 
         giveup(SETL_SYSTEM msg_iter_write_error); 

}

/* last argument integer, others symbol table pointers */

void emitssi(
   int p,                              /* opcode                            */
   symtab_ptr_type o1,                 /* operands                          */
   symtab_ptr_type o2,
   int o3,
   file_pos_type *fp)                  /* source file position              */

{

   if (!USE_INTERMEDIATE_FILES) {

      emit_quad_ptr = get_quad(SETL_SYSTEM_VOID);
      *emit_quad_tail = emit_quad_ptr; 
      emit_quad_tail = &(emit_quad_ptr->q_next);

   } 

   emit_quad_ptr->q_opcode = p; 
   emit_quad_ptr->q_operand[0].q_symtab_ptr = (o1); 
   emit_quad_ptr->q_operand[1].q_symtab_ptr = (o2); 
   emit_quad_ptr->q_operand[2].q_integer = (o3); 
   emit_quad_ptr->q_file_pos.fp_line = (fp)->fp_line; 
   emit_quad_ptr->q_file_pos.fp_column = (fp)->fp_column; 

   if (USE_INTERMEDIATE_FILES)  
      if (fwrite((void *)emit_quad_ptr,sizeof(struct quad_item), 
                 (size_t)1,I1_FILE) != (size_t)1) 
         giveup(SETL_SYSTEM msg_iter_write_error); 

}

#endif 

/*\
 *  \function{close\_emit()}
 *
 *  This function finishes an emit stream.  If we're writing to the
 *  intermediate file we write an end of stream indicator.  If not, we
 *  insert a \verb"NULL" as the last quadruple.
\*/

void close_emit(SETL_SYSTEM_PROTO_VOID)

{

   if (USE_INTERMEDIATE_FILES) {

      /* write a dummy quadruple, to flag the end of the stream */

      emit_record.q_opcode = -1;
      if (fwrite((void *)&emit_record,
                 sizeof(struct quad_item),
                 (size_t)1,
                 I1_FILE) != (size_t)1)
         giveup(SETL_SYSTEM msg_iter_write_error);

   }
   else {

      *emit_quad_tail = NULL;

   }
}

/*\
 *  \function{store\_quads()}
 *
 *  This function saves a quadruple stream in the intermediate file and
 *  frees the memory used by those quadruples.
\*/

void store_quads(
   SETL_SYSTEM_PROTO
   storage_location *location,         /* where quad list should be placed  */
   quad_ptr_type quad_head)            /* head of quadruple list            */

{
quad_ptr_type t1,t2;                   /* temporary looping variables       */

   /* if we're not using intermediate files, just copy the pointer & return */

   if (!USE_INTERMEDIATE_FILES) {

      location->sl_mem_ptr = (void *)quad_head;
      return;

   }

   /* we'll start writing at the end of file */

   if (fseek(I1_FILE,0L,SEEK_END) != 0)
      giveup(SETL_SYSTEM msg_bad_inter_fseek);
   location->sl_file_ptr = ftell(I1_FILE);
   if (location->sl_file_ptr == -1)
      giveup(SETL_SYSTEM msg_bad_inter_fseek);

   /* loop over the quadruple list */

   t1 = quad_head;

   while (t1 != NULL) {

      if (fwrite((void *)t1,
                 sizeof(struct quad_item),
                 (size_t)1,
                 I1_FILE) != (size_t)1)
         giveup(SETL_SYSTEM "Write failure on intermediate file");

      t2 = t1;
      t1 = t1->q_next;
      ((struct table_item *)t2)->ti_union.ti_next = table_next_free;
      table_next_free = (struct table_item *)t2;

   }

   /* write a dummy quadruple, to flag the end of the stream */

   emit_record.q_opcode = -1;
   if (fwrite((void *)&emit_record,
              sizeof(struct quad_item),
              (size_t)1,
              I1_FILE) != (size_t)1)
      giveup(SETL_SYSTEM msg_iter_write_error);

}

/*\
 *  \function{load\_quads()}
 *
 *  This function complements the previous one.  It reloads a quadruple
 *  list from the compiler's intermediate file.
\*/

quad_ptr_type load_quads(
   SETL_SYSTEM_PROTO
   storage_location *location)         /* where quad list should be placed  */

{
quad_ptr_type return_ptr;              /* head of returned list             */
struct table_block *old_head;          /* quadruple table block list head   */
quad_ptr_type *quad_next;              /* work quad nodes                   */

   /* if we're not using intermediate files, just return the memory pointer */

   if (!USE_INTERMEDIATE_FILES)
      return (quad_ptr_type)(location->sl_mem_ptr);

   /* position intermediate file to start of list */

   if (fseek(I1_FILE,location->sl_file_ptr,SEEK_SET) != 0)
      giveup(SETL_SYSTEM msg_bad_inter_fseek);

   /* reload the list */

   quad_next = &return_ptr;

   for (;;) {

      /* load the quadruple from the file */

      if (fread((void *)&emit_record,
              sizeof(struct quad_item),
              (size_t)1,
              I1_FILE) != (size_t)1)
         giveup(SETL_SYSTEM msg_inter_read_error);

      if (emit_record.q_opcode == -1)
         break;

      /* allocate space for the record */

      if (table_next_free == NULL) {

         /* allocate a new block */

         old_head = table_block_head;
         table_block_head = (struct table_block *)
                            malloc(sizeof(struct table_block));
         if (table_block_head == NULL)
            giveup(SETL_SYSTEM msg_malloc_error);
         table_block_head->tb_next = old_head;

         /* link items on the free list */

         for (table_next_free = table_block_head->tb_data;
              table_next_free <=
                 table_block_head->tb_data + QUAD_BLOCK_SIZE - 2;
              table_next_free++) {
            table_next_free->ti_union.ti_next = table_next_free + 1;
         }

         table_next_free->ti_union.ti_next = NULL;
         table_next_free = table_block_head->tb_data;
      }

      /* at this point, we know the free list is not empty */

      *quad_next = &(table_next_free->ti_union.ti_data);
      table_next_free = table_next_free->ti_union.ti_next;

      memcpy((void *)(*quad_next),
             (void *)&emit_record,
             sizeof(struct quad_item));

      quad_next = &((*quad_next)->q_next);

   }

   *quad_next = NULL;

   return return_ptr;

}

/*\
 *  \function{kill\_quads()}
 *
 *  This function frees the memory used by a list of quadruples.
\*/

void kill_quads(
   quad_ptr_type quad_head)            /* list of quadruples to be purged   */

{
quad_ptr_type t1,t2;                   /* temporary looping variables       */

   /* loop over the quadruple list */

   t1 = quad_head;

   while (t1 != NULL) {

      t2 = t1;
      t1 = t1->q_next;
      ((struct table_item *)t2)->ti_union.ti_next = table_next_free;
      table_next_free = (struct table_item *)t2;

   }
}

/*\
 *  \function{print\_quads()}
 *
 *  This function prints a list of quadruples.
\*/

#ifdef DEBUG

void print_quads(
   SETL_SYSTEM_PROTO
   quad_ptr_type quad_head,            /* first quad to be printed          */
   char *title)                        /* listing title                     */

{
int quad_num;                          /* quadruple number                  */
symtab_ptr_type symtab_ptr;            /* symtab operand pointer            */
char print_symbol[16];                 /* truncated symbol for printing     */
int opnd;                              /* used to loop over operands        */
char *p;                               /* temporary looping variable        */

   /* print the title */

   if (title != NULL) {
      fprintf(DEBUG_FILE,"\n%s\n",title);
      for (p = title; *p; p++)
         fputs("-",DEBUG_FILE);
      fputs("\n\n",DEBUG_FILE);
   }

   /* loop over the list of quadruples */

   quad_num = 0;
   while (quad_head != NULL) {

      fprintf(DEBUG_FILE,"%4d  %-15s ",
              quad_num,quad_desc[quad_head->q_opcode]);

      for (opnd = 0; opnd < 3; opnd++) {

         switch (quad_optype[quad_head->q_opcode][opnd]) {

            case QUAD_INTEGER_OP :     case QUAD_LABEL_OP :

                  fprintf(DEBUG_FILE,"%-15d ",
                          quad_head->q_operand[opnd].q_integer);

               break;

            case QUAD_SPEC_OP :        case QUAD_CLASS_OP :
            case QUAD_PROCESS_OP :     case QUAD_SLOT_OP :

               /* just print the name table version of the symbol */

               symtab_ptr = quad_head->q_operand[opnd].q_symtab_ptr;

               if (symtab_ptr == NULL) {
                  fprintf(DEBUG_FILE,"%-15s ","--");
               }
               else {

                  /* build up a junk symbol for temporaries and labels */

#if WATCOM

                  if (symtab_ptr->st_namtab_ptr == NULL) {

                     if (symtab_ptr->st_type == sym_label)
                        sprintf(print_symbol,"$L%p ",symtab_ptr);
                     else
                        sprintf(print_symbol,"$T%p ",symtab_ptr);

                  }

#else

                  if (symtab_ptr->st_namtab_ptr == NULL) {

                     if (symtab_ptr->st_type == sym_label)
                        sprintf(print_symbol,"$L%ld ",(long)symtab_ptr);
                     else
                        sprintf(print_symbol,"$T%ld ",(long)symtab_ptr);

                  }

#endif

                  else {

                     strncpy(print_symbol,
                             (symtab_ptr->st_namtab_ptr)->nt_name,15);
                     print_symbol[15] = '\0';

                  }

                  fprintf(DEBUG_FILE,"%-15s ",print_symbol);

               }

               break;

            default :

               break;

         }
      }

      fprintf(DEBUG_FILE,"  %5d\n",quad_head->q_file_pos.fp_line);

      /* set up for the next quadruple */

      quad_head = quad_head->q_next;
      quad_num++;
   }
}

#endif

