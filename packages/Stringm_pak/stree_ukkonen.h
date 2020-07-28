
#ifndef _STREE_UKKONEN_H_
#define _STREE_UKKONEN_H_

#ifdef __cplusplus
  extern "C" {
#endif

#include "strmat.h"
#include "stree_strmat.h"

int stree_ukkonen_add_string(SUFFIX_TREE tree, char *S, char *Sraw,
                             int M, int strid);

SUFFIX_TREE stree_ukkonen_build(STRING *string, int build_policy,
                                int build_threshold);
SUFFIX_TREE stree_gen_ukkonen_build(STRING **strings, int num_strings,
                                    int build_policy, int build_threshold);

#ifdef __cplusplus
  }
#endif

#endif
