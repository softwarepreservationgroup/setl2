
#ifndef _STRMAT_H_
#define _STRMAT_H_

/*
 * struct.h
 *
 * The data structures implementing the interface between the main
 * program and the algorithm implementations.
 *
 * NOTES:
 *    7/94  -   Original Implementation.   (James Knight)
 *    7/94  -   Redid the output variables for the Boyer-Moore algorithm.
 *              Commented out the SUFFIX_TREE variables (so that the file
 *              can be used before the SUFFIX_TREE data structure has 
 *              been implemented.
 *              (James Knight)
 *    8/94  -   I added all the interface structures and the sequence
 *              structures.  I did all my prototyping in here and I add some
 *              useful Macros.
 *              (Rajdeep Gupta)
 *    8/94  -   Removed all of the clutter Rajdeep put in here, and also
 *              removed all variables which are not currently being used.
 *              Also, updated STRING structure with the title, identifier,
 *              database type, alphabet and description string fields.
 *              (James Knight)
 *    8/94  -   Defined constants for a few alphabets.  Removed interface
 *              specific constants and STRING array structure and moved to
 *              'interface.h'.
 *              (Gene Fodor)
 *    8/94  -   Added fields for the Z construction algorithm, the Z-values
 *              exact matching algorithm, the include file "more.h" and
 *              the constant OUTSIZE.
 *              (James Knight)
 *    9/94  -   Added the suffix tree definition, along with some fields for
 *              the Aho-Corasick algorithm and some suffix tree fields.
 *              (James Knight)
 *    9/94  -   Added the alphabet definition, added the a field for
 *              the user alphabet to the STRING structure
 *              (Gene Fodor)
 *    9/94  -   Added new suffix tree policies, and added a field 
 *              for a miscellaneous pointer to suffix tree definition.
 *              (Gene Fodor)
 *    9/94  -   Added new suffix tree matching field to PROBLEM,
 *              STREE_LEAF, and corresponding fields to SUFFIX_TREE
 *              for generating generalized suffix trees.
 *    7/95  -   Changed some fields/field-names for the Z-values algorithms.
 *              (James Knight)
 *    8/95  -   Redid the SUFFIX_TREE structure using the new implementation.
 *              (James Knight)
 *    3/96  -   Modularized the code  (James Knight)
 *    7/96  -   Finished the modularization  (James Knight)
 */

/*
 * STRING data structure.
 *
 * Contains the sequence and data structures specifically associated with
 * that string.
 *     raw_seq     - the raw characters of the string read from the input
 *                   (should be used when producing output)
 *     sequence    - the mapped characters of the string
 *                   (should be used when executing an algorithm, all strings
 *                    guaranteed to be mapped to the same alphabet)
 *     length      - the length of the string
 *     raw_alpha   - the alphabet of the raw string
 *     alphabet    - the alphabet used to create the mapped string
 *     alpha_size  - the size of the mapped alphabet
 *     db_type     - the type of database which has this string
 *     ident       - an database identifier for retrieving the string
 *     title       - a title or name for the string
 *     desc_string - the complete description for the string
 *                      (a combination of the 'ident', 'title' and 'db_type')
 */

#define IDENT_LENGTH 50
#define TITLE_LENGTH 200

typedef struct {
  char *sequence, *desc_string, *raw_seq;
  int length, alphabet, raw_alpha, db_type;
  int alpha_size;
  char ident[IDENT_LENGTH+1], title[TITLE_LENGTH+1];
} STRING;


/*
 * Common prototypes used by most or all of the strmat files.
 */
#define ERROR 0
#define OK 1

extern char *alpha_names[], *db_names[];

#include "more.h"


/*
 * Prototypes from strmat_main.c
 */
int my_itoalen(int);
char *my_getline(FILE *, int *);

#endif
