
#ifndef _STREE_STRMAT_H_
#define _STREE_STRMAT_H_

#ifdef __cplusplus
  extern "C" {
#endif

#define LINKED_LIST 0
#define SORTED_LIST 1
#define LIST_THEN_ARRAY 2
#define COMPLETE_ARRAY 3

typedef struct stree_intleaf {
  int strid, pos;
  struct stree_intleaf *next;
} SINTLEAF_STRUCT, *STREE_INTLEAF;


typedef struct stree_leaf {
  int isaleaf, id;

  char *edgestr;
  char *rawedgestr;
  int edgelen;

  struct stree_node *parent;
  struct stree_node *next;

  int strid, pos;
} SLEAF_STRUCT, *STREE_LEAF;


typedef struct stree_node {
  int isaleaf, id;

  char *edgestr;
  char *rawedgestr;
  int edgelen;

  struct stree_node *parent;
  struct stree_node *next;

  struct stree_node *suffix_link;

  int isanarray;
  struct stree_node *children;

  STREE_INTLEAF leaves;
} SNODE_STRUCT, *STREE_NODE;


typedef struct {
  STREE_NODE root;
  int num_nodes;

  char **strings;
  char **rawstrings;
  int *lengths, *ids;
  int nextslot, strsize, copyflag;

  int alpha_size;
  int build_type, build_threshold;
  int idents_dirty;

  int tree_size;
  int num_compares, edges_traversed, links_traversed;
  int child_cost, nodes_created, creation_cost;
} STREE_STRUCT, *SUFFIX_TREE;


SUFFIX_TREE stree_new_tree(int alphasize, int copyflag,
                            int build_type, int build_threshold);
void stree_delete_tree(SUFFIX_TREE tree);

void stree_traverse(SUFFIX_TREE tree, int (*preorder_fn)(SUFFIX_TREE ,STREE_NODE),
                    int (*postorder_fn)(SUFFIX_TREE ,STREE_NODE));
void stree_traverse_subtree(SUFFIX_TREE tree, STREE_NODE node,
                            int (*preorder_fn)(SUFFIX_TREE ,STREE_NODE),
							int (*postorder_fn)(SUFFIX_TREE ,STREE_NODE));

int stree_match(SUFFIX_TREE tree, char *T, int N,
                STREE_NODE *node_out, int *pos_out);
int stree_walk(SUFFIX_TREE tree, STREE_NODE node, int pos, char *T, int N,
               STREE_NODE *node_out, int *pos_out);

STREE_NODE stree_find_child(SUFFIX_TREE tree, STREE_NODE node, char ch);
int stree_get_num_children(SUFFIX_TREE tree, STREE_NODE node);
STREE_NODE stree_get_children(SUFFIX_TREE tree, STREE_NODE node);
#define stree_get_next(tree, node) (node)->next

void stree_sort_children(SUFFIX_TREE tree, STREE_NODE node);

#define stree_get_root(tree) ((tree)->root)
#define stree_get_num_nodes(tree) ((tree)->num_nodes)
#define stree_get_parent(tree,node) ((node)->parent)
#define stree_get_suffix_link(tree,node) \
    (!int_stree_isaleaf(tree,node) ? (node)->suffix_link \
                                   : int_stree_get_suffix_link(tree, node))
#define stree_get_edgestr(tree,node)  ((node)->edgestr)
#define stree_get_rawedgestr(tree,node)  ((node)->rawedgestr)
#define stree_get_edgelen(tree,node)  ((node)->edgelen)
#define stree_getch(tree,node)  (*((node)->edgestr))

#define stree_get_ident(tree,node) \
    (!((tree)->idents_dirty) ? (node)->id \
                             : (int_stree_set_idents(tree), (node)->id))

int stree_get_labellen(SUFFIX_TREE tree, STREE_NODE node);
void stree_get_label(SUFFIX_TREE tree, STREE_NODE node, char *buffer,
                     int buflen, int endflag);

int stree_get_num_leaves(SUFFIX_TREE tree, STREE_NODE node);
int stree_get_leaf(SUFFIX_TREE tree, STREE_NODE node, int leafnum,
                   char **string_out, int *pos_out, int *id_out);

void stree_reset_stats(SUFFIX_TREE tree);


/*
 *
 * Internal procedures to use when building and manipulating trees.
 *
 */

int int_stree_insert_string(SUFFIX_TREE tree, char *S, char *Sraw,
                            int M, int strid);
void int_stree_delete_string(SUFFIX_TREE tree, int strid);

STREE_NODE int_stree_convert_leafnode(SUFFIX_TREE tree, STREE_NODE node);

#define int_stree_isaleaf(tree,node)  ((node)->isaleaf)
#define int_stree_has_intleaves(tree,node) \
              (int_stree_isaleaf(tree,node) ? 0 : ((node)->leaves != NULL))
#define int_stree_get_intleaves(tree,node) \
              (int_stree_isaleaf(tree,node) ? NULL : (node)->leaves)

STREE_NODE int_stree_get_suffix_link(SUFFIX_TREE tree, STREE_NODE node);
#define int_stree_get_leafpos(tree,node)  ((node)->pos)

#define int_stree_get_string(tree,id)  ((tree)->strings[(id)])
#define int_stree_get_rawstring(tree,id)  ((tree)->rawstrings[(id)])
#define int_stree_get_length(tree,id)  ((tree)->lengths[(id)])
#define int_stree_get_strid(tree,id)  ((tree)->ids[(id)])

STREE_NODE int_stree_connect(SUFFIX_TREE tree, STREE_NODE parent,
                             STREE_NODE child);
void int_stree_reconnect(SUFFIX_TREE tree, STREE_NODE parent,
                         STREE_NODE oldchild, STREE_NODE newchild);
void int_stree_disc_from_parent(SUFFIX_TREE tree, STREE_NODE parent,
                                STREE_NODE child);
void int_stree_disconnect(SUFFIX_TREE tree, STREE_NODE node);

STREE_NODE int_stree_edge_split(SUFFIX_TREE tree, STREE_NODE node, int len);
void int_stree_edge_merge(SUFFIX_TREE tree, STREE_NODE node);

int int_stree_add_intleaf(SUFFIX_TREE tree, STREE_NODE node,
                          int strid, int pos);
int int_stree_remove_intleaf(SUFFIX_TREE tree, STREE_NODE node,
                             int strid, int pos);

void int_stree_delete_subtree(SUFFIX_TREE tree, STREE_NODE node);

int int_stree_walk_to_leaf(SUFFIX_TREE tree, STREE_NODE node, int pos,
                           char *T, int N, STREE_NODE *node_out, int *pos_out);

void int_stree_set_idents(SUFFIX_TREE tree);

STREE_INTLEAF int_stree_new_intleaf(SUFFIX_TREE tree, int strid, int pos);
STREE_LEAF int_stree_new_leaf(SUFFIX_TREE tree, int strid, int edgepos,
                              int leafpos);
STREE_NODE int_stree_new_node(SUFFIX_TREE tree, char *edgestr,
                              char *rawedgestr, int edgelen);
void int_stree_free_intleaf(SUFFIX_TREE tree, STREE_INTLEAF ileaf);
void int_stree_free_leaf(SUFFIX_TREE tree, STREE_LEAF leaf);
void int_stree_free_node(SUFFIX_TREE tree, STREE_NODE node);

#ifdef __cplusplus
	}
#endif


#endif
