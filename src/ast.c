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
 *  \package{The Abstract Syntax Tree}
 *
 *  This package contains several low-level functions which manipulate
 *  abstract syntax tree nodes.  The table is dynamically allocated in
 *  blocks, like most of the other tables in the compiler.  The functions
 *  which allocate and free nodes are fairly standard.
 *
 *  There are a few unusual things here which we should explain.  We have
 *  gone to some effort to make AST nodes as small as possible.  There
 *  are only two pointers per AST node, one overloaded pointer for
 *  children and one for the next in a list.
 *
 *  The searches are also a little unusual.  We prefer to iterate over
 *  lists at a given level, but use recursion to travel down the trees.
 *
 *  We have also placed in this package some functions which save and
 *  restore abstract syntax trees in a compiler temporary file.  We feel
 *  that this function is highly dependent upon the form of ast nodes, so
 *  should properly be here.
 *
 *  \texify{ast.h}
 *
 *  \packagebody{Abstract Syntax Tree}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "ast.h"                       /* abstract syntax tree              */

#ifdef PLUGIN
#define fprintf plugin_fprintf
#define fputs   plugin_fputs
#endif


/* constants */

#define NEXT_CHILD   1                 /* node is 'next' child of parent    */
#define CHILD_CHILD  2                 /* node is 'child' of parent         */

/* performance tuning constants */

#define AST_BLOCK_SIZE       200       /* ast block size                    */
#define ASTREC_BLOCK_SIZE     10       /* ast record stack block size       */

/* generic table item structure (ast use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct ast_item ti_data;         /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[AST_BLOCK_SIZE];
                                       /* array of table items              */
};

/* intermediate file ast record */

struct ast_record {
   struct ast_item ar_ast_item;        /* complete ast item                 */
   int ar_self_index;                  /* nodes dfs number                  */
   int ar_parent_index;                /* parent's dfs number               */
   int ar_which_child;                 /* child type (next or child)        */
};

/* package-global data */

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */
static int ast_dfs_number;             /* global dfs number for tree        */
                                       /* traversals                        */
static struct ast_record *astrec_stack = NULL;
                                       /* stack of ast records              */
static int astrec_top = -1;            /* stack top                         */
static int astrec_max = 0;             /* size of stack                     */

/* macro to return a new ast record stack item */

#ifdef TSAFE
#define get_astrec \
   ((astrec_top < astrec_max - 1) ? ++astrec_top : alloc_astrec(plugin_instance))
#else
#define get_astrec \
   ((astrec_top < astrec_max - 1) ? ++astrec_top : alloc_astrec())
#endif

/* forward declarations */

static void save_subtree(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* recursively store a tree          */
static int alloc_astrec(SETL_SYSTEM_PROTO_VOID);
                                       /* allocate an ast record stack      */
                                       /* block                             */
#ifdef DEBUG
static void print_subtree(SETL_SYSTEM_PROTO ast_ptr_type,int);
                                       /* print an AST subtree              */
#endif

/*\
 *  \function{init\_ast()}
 *
 *  This procedure initializes the abstract syntax tree.  All we do is
 *  free all the allocated blocks.
\*/

void init_ast()

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
 *  \function{get\_ast()}
 *
 *  This procedure allocates an AST node.  It is just like most of the
 *  other dynamic table allocation functions in the compiler.
\*/

ast_ptr_type get_ast(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *old_head;          /* name table block list head        */
ast_ptr_type return_ptr;               /* return pointer                    */

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
              table_block_head->tb_data + AST_BLOCK_SIZE - 2;
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

   clear_ast(return_ptr);

   return return_ptr;

}

/*\
 *  \function{free\_ast()}
 *
 *  This function is the complement to \verb"get_ast()".  All we do is
 *  push the passed AST pointer on the free list.
\*/

void free_ast(
   ast_ptr_type discard)               /* item to be discarded              */

{

   ((struct table_item *)discard)->ti_union.ti_next = table_next_free;
   table_next_free = (struct table_item *)discard;

}

/*\
 *  \function{store\_ast()}
 *
 *  These two functions save an abstract syntax tree in the compiler's
 *  intermediate file.  They use recursion to save the tree.  In order to
 *  reconstruct the tree correctly later, we save the nodes in DFS
 *  postorder, so when we read the records back in we will read children
 *  before parents.
 *
 *  The search here is a little tricky.  We perform a depth first search,
 *  traversing \verb"ast_next" chains first, then \verb"ast_child_ast"'s.
 *  The tricky part is that we do not use recursion to traverse the
 *  \verb"ast_next" chains.  Instead, we just traverse these chains, but
 *  flip the pointers so that we can return up the chain.  This seems
 *  like a lot of useless work, but we expect ast's to be fairly flat,
 *  but wide, so this eliminates a lot of recursive calls.
\*/

void store_ast(
   SETL_SYSTEM_PROTO
   storage_location *location,         /* storage location for AST          */
   ast_ptr_type ast_root)              /* ast_root of ast to be stored      */

{

   /* if we're not using intermediate files, just copy the pointer & return */

   if (!USE_INTERMEDIATE_FILES) {

      location->sl_mem_ptr = (void *)ast_root;
      return;

   }

   /* we'll start writing at the end of file */

   if (fseek(I1_FILE,0L,SEEK_END) != 0)
      giveup(SETL_SYSTEM msg_bad_inter_fseek);
   location->sl_file_ptr = ftell(I1_FILE);
   if (location->sl_file_ptr == -1)
      giveup(SETL_SYSTEM msg_bad_inter_fseek);
   ast_dfs_number = 0;
   save_subtree(SETL_SYSTEM ast_root,0);

}

static void save_subtree(
   SETL_SYSTEM_PROTO
   ast_ptr_type ast_root,              /* subtree to be written             */
   int parent)                         /* parent index                      */

{
struct ast_record astrec;              /* work record                       */
int node_index;                        /* current node index                */
ast_ptr_type t1,t2,t3;                 /* temporary looping variables       */

   /* don't do anything for NULL pointers (shouldn't happen) */

   if (ast_root == NULL)
      return;

   /* first we traverse ast_next chains, reversing pointers */

   t1 = NULL;
   t2 = ast_root;
   t3 = t2->ast_next;

   while (t2 != NULL) {

      ast_dfs_number++;

      t2->ast_next = t1;
      t1 = t2;
      t2 = t3;
      if (t3 != NULL)
         t3 = t3->ast_next;
   }

   /*
    *  At this point, we've followed a chain of ast_next nodes to a leaf.
    *  We follow the chain backwards, processing children also.
    */

   node_index = ast_dfs_number + 1;

   while (t1 != NULL) {

      node_index--;

      /* save children */

      switch (t1->ast_type) {

         case ast_namtab :
         case ast_symtab :

            break;

         default :

            if (t1->ast_child.ast_child_ast != NULL)
               save_subtree(SETL_SYSTEM t1->ast_child.ast_child_ast,node_index);
            break;

      }

      /* build an ast record */

      memcpy((void *)&(astrec.ar_ast_item),
             (void *)t1,
             sizeof(struct ast_item));
      astrec.ar_self_index = node_index;
      if (t1 == ast_root) {
         astrec.ar_parent_index = parent;
         astrec.ar_which_child = CHILD_CHILD;
      }
      else {
         astrec.ar_parent_index = node_index - 1;
         astrec.ar_which_child = NEXT_CHILD;
      }
      if (fwrite((void *)&astrec,
                 sizeof(struct ast_record),
                 (size_t)1,
                 I1_FILE) != (size_t)1)
         giveup(SETL_SYSTEM "Write failure on intermediate file");

      /* free the node and set up for the next one */

      t2 = t1;
      t1 = t1->ast_next;
      ((struct table_item *)t2)->ti_union.ti_next = table_next_free;
      table_next_free = (struct table_item *)t2;

   }
}

/*\
 *  \function{load\_ast()}
 *
 *  This function complements the previous pair.  It just reloads an ast
 *  from the compiler's intermediate file.
\*/

ast_ptr_type load_ast(
   SETL_SYSTEM_PROTO
   storage_location *location)         /* location where AST is stored      */

{
struct ast_record astrec;              /* work record                       */
ast_ptr_type ast_ptr;                  /* work ast node                     */

   /* if we're not using intermediate files, just return the memory pointer */

   if (!USE_INTERMEDIATE_FILES)
      return (ast_ptr_type)(location->sl_mem_ptr);

   /* position intermediate file to start of tree */

   if (fseek(I1_FILE,location->sl_file_ptr,SEEK_SET) != 0)
      giveup(SETL_SYSTEM msg_bad_inter_fseek);

   /* reconstruct the tree */

   astrec_top = -1;
   do {

      if (fread((void *)&astrec,
                sizeof(struct ast_record),
                (size_t)1,
                I1_FILE) != (size_t)1)
         giveup(SETL_SYSTEM msg_inter_read_error);

      /* attach children */

      astrec.ar_ast_item.ast_next = NULL;
      while (astrec_top >= 0 &&
             astrec_stack[astrec_top].ar_parent_index ==
                astrec.ar_self_index) {

         ast_ptr = get_ast(SETL_SYSTEM_VOID);
         memcpy((void *)ast_ptr,
                (void *)&(astrec_stack[astrec_top].ar_ast_item),
                sizeof(struct ast_item));

         if (astrec_stack[astrec_top].ar_which_child == NEXT_CHILD)
            astrec.ar_ast_item.ast_next = ast_ptr;
         else
            astrec.ar_ast_item.ast_child.ast_child_ast = ast_ptr;

         astrec_top--;

      }

      /* push this item on the stack */

      get_astrec;
      memcpy((void *)(astrec_stack + astrec_top),
             (void *)&astrec,
             sizeof(struct ast_record));

   } while (astrec.ar_self_index != 1);

   /* finally, remove the ast_root from the stack */

   ast_ptr = get_ast(SETL_SYSTEM_VOID);
   memcpy((void *)ast_ptr,
          (void *)&(astrec_stack[astrec_top].ar_ast_item),
          sizeof(struct ast_item));

   return ast_ptr;

}

/*\
 *  \function{alloc\_astrec()}
 *
 *  This function allocates a block in the ast record stack.   This table
 *  is organized as an `expanding array'.  That is, we allocate an array
 *  of a given size, then when that is exceeded, we allocate a larger
 *  array and copy the smaller to the larger.  This makes allocations
 *  slower than for the other tables, but this stack should never get
 *  very big.
 *
 *  Notice: this function is only called indirectly, through the macro
 *  \verb"get_astrec()".  Most of the time, all we need to do to allocate
 *  a new item is to increment the stack top.  We therefore defined a
 *  macro which did that, and called this function on a stack overflow.
\*/

static int alloc_astrec(SETL_SYSTEM_PROTO_VOID)

{
struct ast_record *temp_astrec_stack;  /* temporary ast record stack        */

   /* expand the table */

   temp_astrec_stack = astrec_stack;
   astrec_stack = (struct ast_record *)malloc((size_t)(
              (astrec_max + ASTREC_BLOCK_SIZE) * sizeof(struct ast_record)));
   if (astrec_stack == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* copy the old table to the new table, and free the old table */

   if (astrec_max > 0) {

      memcpy((void *)astrec_stack,
             (void *)temp_astrec_stack,
             (size_t)(astrec_max * sizeof(struct ast_record)));

      free(temp_astrec_stack);

   }

   astrec_max += ASTREC_BLOCK_SIZE;

   return ++astrec_top;

}

/*\
 *  \function{kill\_ast()}
 *
 *  This function frees the memory for an abstract syntax tree.  It
 *  purges the tree completely, so it should only be called after another
 *  intermediate form of the code has been generated, or we have found
 *  some errors, and know that we do not need to generate code.
\*/

void kill_ast(
   ast_ptr_type ast_root)              /* tree to be released               */

{
ast_ptr_type old_ptr;                  /* pointer to deleted node           */

   /* traverse the next chains, and call function recursively for children */

   while (ast_root != NULL) {

      switch (ast_root->ast_type) {

         case ast_namtab :
         case ast_symtab :

            break;

         default :

            kill_ast(ast_root->ast_child.ast_child_ast);
            break;

      }

      /* free the node and set up for the next one */

      old_ptr = ast_root;
      ast_root = ast_root->ast_next;
      ((struct table_item *)old_ptr)->ti_union.ti_next = table_next_free;
      table_next_free = (struct table_item *)old_ptr;

   }
}

/*\
 *  \function{print\_ast()}
 *
 *  These two functions print an abstract syntax tree rooted at the
 *  passed argument.  The first is just an entry function, and the second
 *  is a recursive function which expects a root and an indentation
 *  level.
\*/

#ifdef DEBUG

void print_ast(
   SETL_SYSTEM_PROTO
   ast_ptr_type ast_root,              /* ast_root of tree to be printed    */
   char *title)                        /* listing title                     */

{
char *p;                               /* temporary looping variable        */

   if (ast_root == NULL)
      return;

   /* print the title */

   if (title != NULL) {
      fprintf(DEBUG_FILE,"\n%s\n",title);
      for (p = title; *p; p++)
         fputs("-",DEBUG_FILE);
      fputs("\n\n",DEBUG_FILE);
   }

   print_subtree(SETL_SYSTEM ast_root,0);

}

static void print_subtree(
   SETL_SYSTEM_PROTO
   ast_ptr_type ast_root,              /* ast_root of tree to be printed    */
   int indent)                         /* indentation level                 */

{
ast_ptr_type ast_ptr;                  /* used to loop over nodes           */
symtab_ptr_type symtab_ptr;            /* work symbol table pointer         */
char print_symbol[16];                 /* truncated symbol for printing     */
int i;                                 /* temporary looping variable        */

#ifdef TRAPS

static int max_ast_type = -1;          /* number of ast types               */

   if (max_ast_type == -1) {

      for (max_ast_type = 0; ast_desc[max_ast_type] != NULL; max_ast_type++);
      max_ast_type--;

   }

#endif

   if (ast_root == NULL)
      return;

   /* we loop over nodes at the current level */

   for (ast_ptr = ast_root; ast_ptr != NULL; ast_ptr = ast_ptr->ast_next) {

      /* space over the indentation level */

      for (i = 0; i < indent; i++)
         fputs("   ",DEBUG_FILE);

      /* print this node */

#ifdef TRAPS

      if (ast_ptr->ast_type < 0 || ast_ptr->ast_type > max_ast_type) {
         trap(__FILE__,__LINE__,msg_bad_ast_node,ast_ptr->ast_type);
      }

#endif

      fprintf(DEBUG_FILE,"%s",ast_desc[ast_ptr->ast_type]);

      switch (ast_ptr->ast_type) {

         case ast_namtab :

            fprintf(DEBUG_FILE," : %s\n",
                    (ast_ptr->ast_child.ast_namtab_ptr)->nt_name);
            break;

         case ast_symtab :

            symtab_ptr = ast_ptr->ast_child.ast_symtab_ptr;

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

            fprintf(DEBUG_FILE," : %-15s\n",print_symbol);

            break;

         default :

            fputs("\n",DEBUG_FILE);
            print_subtree(SETL_SYSTEM ast_ptr->ast_child.ast_child_ast,indent+1);
            break;

      }
   }
}

#endif

