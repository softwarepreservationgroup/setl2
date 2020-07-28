
#ifndef _STRMAT_MATCH_H_
#define _STRMAT_MATCH_H_

#define ONESEQ_EXACT 0
#define ONESEQ_APPROX 1
#define SET_EXACT 2
#define SET_APPROX 3
#define TEXT_SET_EXACT 4

typedef struct matchnode {
  int type, id, textid;
  int lend, rend, score;
  struct matchnode *next;
} MATCH_NODE, *MATCHES;

MATCH_NODE *alloc_match(void);
void free_matches(MATCHES list);

#endif
