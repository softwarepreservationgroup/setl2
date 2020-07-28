/*
 * stree_ukkonen.c
 *
 * The implementation of Ukkonen's suffix tree algorithm, for use with
 * strmat's suffix tree implementation.
 *
 * NOTES:
 *    9/95  -  Separated Ukkonen's algorithm from stree.c as part
 *             of suffix tree reimplementation  (James Knight)
 *    4/96  -  Modularized the code  (James Knight)
 *    7/96  -  Finished the modularization  (James Knight)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strmat.h"
#include "stree_strmat.h"
#include "stree_ukkonen.h"


/*
 * stree_ukkonen_add_string
 *
 * Use Ukkonen's suffix tree construction algorithm to add a string
 * to a suffix tree.
 *
 * Parameters:  tree  -  a suffix tree
 *              S     -  the string to add
 *              Sraw  -  the raw version of the string
 *              M     -  the string length
 *              strid -  the string identifier
 *
 * Returns:  non-zero on success, zero on error.
 */
int stree_ukkonen_add_string(SUFFIX_TREE tree, char *S, char *Sraw,
                             int M, int strid)
{
  int i, j, g, h, gprime, edgelen, id;
  char *edgestr;
  STREE_NODE node, lastnode, root, child, parent;
  STREE_LEAF leaf;

  id = int_stree_insert_string(tree, S, Sraw, M, strid);
  if (id == -1)
    return 0;

  /*
   * Run Ukkonen's algorithm to add the string to the suffix tree.
   *
   * This implementation differs from the book description in 
   * several ways:
   *    1) The algorithm begins at the root of the tree and
   *       constructs I_{1} (the implicit suffix tree for just
   *       the first character) using the normal extension rules.
   *       The reason for this is to be able to handle generalized
   *       suffix trees, where that first character may already
   *       be in the tree.
   *    2) The algorithm inserts the complete suffix into the
   *       tree when extension rule 2 applies (rather than deal
   *       with the business of "increasing" suffices on the leaf
   *       nodes).
   *    3) All of the offsets into the sequence, and the phases of
   *       the algorithm, use the C array indices of 0 to M-1, not 1 to M.
   *    4) The algorithm handles the conversion from implicit tree
   *       to true suffix tree by adding an additional "phase" M.  In
   *       that phase, the leaves that normally would be added because
   *       of the end of string symbol '$' are added (without resorting
   *       to the use of a special symbol).
   *    5) The constructed suffix tree only has suffix links in
   *       the internal nodes of the tree (to save space).  However,
   *       the stree_get_suffix_link function will return the suffix links
   *       even for leaves (it computes the leaves' suffix links on the
   *       fly).
   */
  root = stree_get_root(tree);
  node = lastnode = root;
  g = 0;
  edgelen = 0;
  edgestr = NULL;

  for (i=0,j=0; i <= M; i++)  {
    for ( ; j <= i && j < M; j++) {
      /*
       * Perform the extension from S[j..i-1] to S[j..i].  One of the
       * following two cases holds:
       *    a) g == 0, node == root and i == j.
       *         (meaning that in the previous outer loop,
       *          all of the extensions S[1..i-1], S[2..i-1], ...,
       *          S[i-1..i-1] were done.)
       *    b) g > 0, node != root and the string S[j..i-1]
       *       ends at the g'th character of node's edge.
       */
      if (g == 0 || g == edgelen) {
        if (i < M) {
          /*
           * If an outgoing edge matches the next character, move down
           * that edge.
           */
#ifdef STATS
          tree->num_compares++;
#endif
          if ((child = stree_find_child(tree, node, S[i])) != NULL) {
#ifdef STATS
            tree->edges_traversed++;
#endif
            node = child;
            g = 1;
            edgestr = stree_get_edgestr(tree, node);
            edgelen = stree_get_edgelen(tree, node);
            break;
          }

          /*
           * Otherwise, add a new leaf out of the current node.
           */
          if ((leaf = int_stree_new_leaf(tree, id, i, j)) == NULL ||
              (node = int_stree_connect(tree, node,
                                        (STREE_NODE) leaf)) == NULL) {
            if (leaf != NULL)
              int_stree_free_leaf(tree, leaf);
            return 0;
          }

          tree->num_nodes++;
        }
        else {
          /*
           * If i == M, then the suffix ends inside the tree, so
           * add a new intleaf at the current node.
           */
          if (int_stree_isaleaf(tree, node) &&
              (node = int_stree_convert_leafnode(tree, node)) == NULL)
            return 0;

          if (!int_stree_add_intleaf(tree, node, id, j))
            return 0;
        }

        if (lastnode != root && lastnode->suffix_link == NULL)
          lastnode->suffix_link = node;
        lastnode = node;
      }
      else {
        /*
         * g > 0 && g < edgelen, and so S[j..i-1] ends in the middle
         * of some edge.
         *
         * If the next character in the edge label matches the next
         * input character, keep moving down that edge.  Otherwise,
         * split the edge at that point and add a new leaf for the
         * suffix.
         */
#ifdef STATS
        tree->num_compares++;
#endif
        if (i < M && S[i] == edgestr[g]) {
          g++;
          break;
        }

        if ((node = int_stree_edge_split(tree, node, g)) == NULL)
          return 0;

        edgestr = stree_get_edgestr(tree, node);
        edgelen = stree_get_edgelen(tree, node);

        if (i < M) {
          if ((leaf = int_stree_new_leaf(tree, id, i, j)) == NULL ||
              (node = int_stree_connect(tree, node,
                                        (STREE_NODE) leaf)) == NULL) {
            if (leaf != NULL)
              int_stree_free_leaf(tree, leaf);
            return 0;
          }

          tree->num_nodes++;
        }
        else {
          /*
           * If i == M, then the suffix ends inside the tree, so
           * add a new intleaf at the node created by the edge split.
           */
          if (int_stree_isaleaf(tree, node) &&
              (node = int_stree_convert_leafnode(tree, node)) == NULL)
            return 0;

          if (!int_stree_add_intleaf(tree, node, id, j))
            return 0;
        }

        if (lastnode != root && lastnode->suffix_link == NULL)
          lastnode->suffix_link = node;
        lastnode = node;
      }

      /* Now, having extended S[j..i-1] to S[j..i] by rule 2, find where
       * S[j+1..i-1] is.  Note that the values of node and g have not
       * changed in the above code (since stree_edge_split splits the
       * node on the g'th character), so either g == 0 and node == root
       * or the string S[j..i-1] ends at the g-1'th character of node's
       * edge (and node is not the root).
       */
      if (node == root)
        ;
      else if (g == edgelen && node->suffix_link != NULL) {
        node = node->suffix_link;

#ifdef STATS
        tree->links_traversed++;
#endif

        edgestr = stree_get_edgestr(tree, node);
        edgelen = stree_get_edgelen(tree, node);

        g = edgelen;
        continue;
      }
      else {
        /*
         * Move across the suffix link of the parent (unless the
         * parent is the root).
         */
        parent = stree_get_parent(tree, node);
        if (parent != root) {
          node = parent->suffix_link;

#ifdef STATS
          tree->links_traversed++;
#endif
        }
        else {
          node = root;
          g--;
        }
        edgelen = stree_get_edgelen(tree, node);

        /*
         * Use the skip/count trick to move g characters down the tree.
         */
        h = i - g;
        while (g > 0) {
          node = stree_find_child(tree, node, S[h]);

#ifdef STATS
          tree->num_compares++;
          tree->edges_traversed++;
#endif

          gprime = stree_get_edgelen(tree, node);
          if (gprime > g)
            break;

          g -= gprime;
          h += gprime;
        }

        edgelen = stree_get_edgelen(tree, node);
        edgestr = stree_get_edgestr(tree, node);

        /*
         * After the walk down, either "g > 0" and S[j+1..i-1] ends g
         * characters down the edge to `node', or "g == 0" and S[j+1..i-1]
         * really ends at `node' (i.e., all of the characters on the edge
         * label to `node' match the end of S[j+1..i-1]).
         *
         * If "g > 0" or "g == 0" but `node' points to a leaf (which could
         * happen if S[j+1..i-1] was the suffix of a previously added
         * string), then we must delay the setting of the suffix link
         * until a node has been created.  (With the suffix tree data 
         * structure, no suffix links can safely point to leaves of the
         * tree because a leaf may be converted into a node at some future
         * time.)
         */
        if (g == 0) {
          if (lastnode != root && !int_stree_isaleaf(tree, node) &&
              lastnode->suffix_link == NULL) {
            lastnode->suffix_link = node;
            lastnode = node;
          }

          if (node != root)
            g = edgelen;
        }
      }
    }
  }

  return 1;
}


/*
 * stree_ukkonen_build
 *
 * Build a suffix tree for a single string.
 *
 * Parameters:  string           -  the string
 *              build_policy     -  what type of build policy to use
 *              build_threshold  -  for LIST_THEN_ARRAY, when to move from
 *                                  linked list to complete array
 *
 * Returns:  the suffix tree, or NULL on an error.
 */
SUFFIX_TREE stree_ukkonen_build(STRING *string, int build_policy,
                                int build_threshold)
{
  SUFFIX_TREE tree;

  if (string == NULL || string->sequence == NULL || string->length == 0)
    return NULL;
  
  tree = stree_new_tree(string->alpha_size, 1, build_policy, build_threshold);
  if (tree == NULL)
    return NULL;

  if (stree_ukkonen_add_string(tree, string->sequence,
                               string->raw_seq, string->length, 1) < 1) {
    stree_delete_tree(tree);
    return NULL;
  }

  return tree;
}


/*
 * stree_gen_ukkonen_build
 *
 * Build a generalized suffix tree for multiple strings.
 *
 * Parameters:  strings          -  the strings
 *              num_strings      -  the number of strings
 *              build_policy     -  what type of build policy to use
 *              build_threshold  -  for LIST_THEN_ARRAY, when to move from
 *                                  linked list to complete array
 *
 * Returns:  the suffix tree, or NULL on an error.
 */
SUFFIX_TREE stree_gen_ukkonen_build(STRING **strings, int num_strings,
                                    int build_policy, int build_threshold)
{
  int i;
  SUFFIX_TREE tree;

  if (strings == NULL || num_strings == 0)
    return NULL;

  tree = stree_new_tree(strings[0]->alpha_size, 0,
                        build_policy, build_threshold);
  if (tree == NULL)
    return NULL;

  for (i=0; i < num_strings; i++) {
    if (stree_ukkonen_add_string(tree, strings[i]->sequence,
                                 strings[i]->raw_seq,
                                 strings[i]->length, i+1) < 1) {
      stree_delete_tree(tree);
      return NULL;
    }
  }

  return tree;
}
