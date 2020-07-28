/*\
 *
 *
 *  MIT License
 * 
 *  Copyright (c) 2001 Salvatore Paxia
 * 
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 *  This is a String Matching Native Package for SETL2
 *
 */


/* SETL2 system header files */

#include "macros.h"

#include "ac.h"                        /* Include Aho-Corasick algorithm    */
#include "stree_strmat.h"                     /* Include Suffix trees              */
#include "stree_ukkonen.h"                     /* Include Suffix trees              */
#include "strmat_match.h"

struct setl_pat {                      /* Native Object Structure           */
   int32 use_count;                    /* Reference Count                   */
   int32 type;                         /* Encodes Type and Subtype          */
   int32 len;                          
   char *pattern;
   void *ptr;
   void *ptr2;
};

/* constants */

#define BM_PATTERN        1            /* Boyer-Moore pattern               */
#define KMP_PATTERN       2            /* Knuth-Moore-Pratt pattern         */
#define AC_PATTERN        3            /* Aho-Corasick pattern              */
#define ST_OBJECT         4            /* Suffix Tree object                */
#define PW_SCORES         5            /* Boyer-Moore compiled pattern      */


static int32 pat_type;                 /* Store the type assigned to us by  */
                                       /* the SETL runtime                  */

static void internal_destructor(struct setl_pat *spec)
{
int subtyp;
AC_STRUCT *ac;
SUFFIX_TREE st;

    if ((spec!=NULL)&&((spec->type&65535)==pat_type)) {
    subtyp = spec->type  / 65535;
       if (subtyp==AC_PATTERN) {
           ac = (AC_STRUCT*)(spec->ptr);
           if (ac->T) free((ac->T)+1);
           ac_free(ac);
       } else if (subtyp==ST_OBJECT) {
           st = (SUFFIX_TREE)(spec->ptr);
           stree_delete_tree(st);
       } else if (subtyp==PW_SCORES) {
  	   if (spec->ptr)     free(spec->ptr);
       } else {
	   if (spec->pattern) free(spec->pattern);
  	   if (spec->ptr)     free(spec->ptr);
      	   if (spec->ptr2)     free(spec->ptr2);
       }
    }

}

void check_arg(
  SETL_SYSTEM_PROTO
  specifier *argv,                  
  int param,
  int type,
  char *typestr,
  char *routine)
{

   if (argv[param].sp_form != type)
      abend(SETL_SYSTEM msg_bad_arg,typestr,param+1,routine,
            abend_opnd_str(SETL_SYSTEM argv+param));

}


SETL_API int32 STRINGM__INIT(
   SETL_SYSTEM_PROTO_VOID)
{
   pat_type=register_type(SETL_SYSTEM "pattern maching",internal_destructor);
   if (pat_type==0) return 1;
   return 0;

}


/*
 * Karp Rabin Fingerprinting 
 */


#define REHASH(a, b, h) (((h-a*d)<<1)+b)


SETL_API void KR(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)

TUPLE_CONSTRUCTOR(ca)

unsigned char *key;                     /* system key                        */
unsigned char *pattern;                 /* system key                        */

int head,la,lb;


   check_arg(SETL_SYSTEM argv,0,ft_string,"string","kr");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","kr");

   ITERATE_STRING_BEGIN(sa,argv[0]);
   la = STRING_LEN(argv[0]);

   key = (char *)malloc((size_t)(la + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,key);

   ITERATE_STRING_BEGIN(sb,argv[1]);

   lb = STRING_LEN(argv[1]);
   pattern = (char *)malloc((size_t)(lb + 1));
   if (pattern == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sb,pattern);

   head = 0;
   {
     int hy, hx, d, i;

  /* Preprocessing */
  /* computes d = 2^(plen-1) with the left-shift operator */
  d = 1;
  for (i = 1; i < lb ; i++)
    d = (d << 1);

  hy = hx = 0;
  for (i = 0; i < lb; i++)
    {
      hx = ((hx << 1) + pattern[i]);
      hy = ((hy << 1) + key[i]);
    }

  /* Searching */
  for (i = 0; i <= (la-lb); i++)
    {
      if (hy == hx && (memcmp (&key[i], pattern, lb) == 0)) {
           specifier s;

           if (!head) {
              TUPLE_CONSTRUCTOR_BEGIN(ca);
              head = 1;
           }
   
           s.sp_form = ft_short;
           s.sp_val.sp_short_value = (i+1);
          
           TUPLE_ADD_CELL(ca,&s);


      }
      hy = REHASH (key[i], key[i+lb], hy);
    }
  }

  free(pattern);
  free(key);

   if (head) {
      TUPLE_CONSTRUCTOR_END(ca);
      unmark_specifier(target);
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
      return;
   }

   unmark_specifier(target);
   target->sp_form = ft_omega;

}



/*
 *
 */

SETL_API void KMP_COMPILE_IN(
  SETL_SYSTEM_PROTO
  char *pattern,
  int m,
  int32 *kmp_next)
{
int i,j;

   i=0;
   j = kmp_next[0]=-1;
   while (i<m) {
	while ( j>-1 && pattern[i]!= pattern[j]) j = kmp_next[j];
        i++;j++;
        if (pattern[i]==pattern[j]) kmp_next[i]=kmp_next[j];
        else kmp_next[i]=j;
   }
}

SETL_API void KMP_COMPILE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_pat *A; 
STRING_ITERATOR(sb)

unsigned char *pattern;                 /* system key                        */
int32 *kmp_next;
int m;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","kmp_compile");

   m = STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sb,argv[0]);

   pattern = (char *)malloc((size_t)(m + 1));
   if (pattern == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sb,pattern);

   A = (struct setl_pat *)(malloc(sizeof(struct setl_pat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   kmp_next = (int32 *)malloc((size_t)(sizeof(int)*(m+1)));
   if (kmp_next == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = pat_type+65536*KMP_PATTERN;

   A->len = m;
   A->pattern = pattern;
   A->ptr = (void*)kmp_next;
   A->ptr2 = NULL;

   KMP_COMPILE_IN(SETL_SYSTEM pattern,m,kmp_next);


   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;

}

SETL_API void KMP_EXEC_IN(
  SETL_SYSTEM_PROTO
  char *key,
  int n,
  char *pattern,
  int m,
  int32 *kmp_next,
  specifier *target,
  int free_pattern)
{
int i,j;
TUPLE_CONSTRUCTOR(ca)
int head;

   head = i = j = 0;
   while (i<n) {
	while ( j>-1 && key[i]!= pattern[j]) j = kmp_next[j];
        i++;j++;
	if (j>=m) {
            specifier s;

           if (!head) {
              TUPLE_CONSTRUCTOR_BEGIN(ca);
              head = 1;
           }
   
           s.sp_form = ft_short;
           s.sp_val.sp_short_value = (i-j+1);
          
           TUPLE_ADD_CELL(ca,&s);

	   j = kmp_next[j];
        }

   }

   if (free_pattern) {
     free(pattern);
     free(key);
     free(kmp_next);
   }

   if (head) {
      TUPLE_CONSTRUCTOR_END(ca);
      unmark_specifier(target);
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
      return;
   }

   unmark_specifier(target);
   target->sp_form = ft_omega;

}

SETL_API void KMP_EXEC(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_pat *A; /* w */ 
STRING_ITERATOR(sa)
char *key;                             /* system key                        */
int n;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","kmp_exec");

   n=STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sa,argv[0]);

   key = (char *)malloc((size_t)(n + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,key);

   if ((argv[1].sp_form != ft_opaque)||
       (((argv[1].sp_val.sp_opaque_ptr->type)&65535)!=pat_type))
      abend(SETL_SYSTEM msg_bad_arg,"string matching",2,"kmp_exec",
         abend_opnd_str(SETL_SYSTEM argv+1));

   A = (struct setl_pat *)(argv[1].sp_val.sp_opaque_ptr);

   KMP_EXEC_IN(SETL_SYSTEM key,n,A->pattern,A->len,(int32*)A->ptr,
	target,NO);

}

SETL_API void KMP(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)

TUPLE_CONSTRUCTOR(ca)

unsigned char *key;                    /* system key                        */
char *pattern;                         /* system key                        */
int32 *kmp_next;

int i,j,n,m,head;


   check_arg(SETL_SYSTEM argv,0,ft_string,"string","kmp");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","kmp");

   n=STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sa,argv[0]);

   key = (char *)malloc((size_t)(n + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,key);

   m=STRING_LEN(argv[1]);
   ITERATE_STRING_BEGIN(sb,argv[1]);

   pattern = (char *)malloc((size_t)(m + 1));
   if (pattern == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sb,pattern);

   kmp_next = (int32 *)malloc((size_t)(sizeof(int)*(m+1)));
   if (kmp_next == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);


   KMP_COMPILE_IN(SETL_SYSTEM pattern,m,kmp_next);
   KMP_EXEC_IN(SETL_SYSTEM key,n,pattern,m,kmp_next,target,YES);

}


/*
 *   Boyer-Moore
 */

SETL_API void BM_COMPILE_IN(
  SETL_SYSTEM_PROTO
  char *pattern,
  int m,
  int32 *bm_gs,
  int32 *bm_bc)
{
int i,j;
int32* f;
int32 p;

   memset(bm_gs,0,2*(m+1)*sizeof(int32));
   f = bm_gs+(sizeof(int32)*(m+1));

   f[m]=j=m+1;
   for (i=m;i>0;i--) {
	while (j<=m && pattern[i-1]!=pattern[j-1]) {
	   if (bm_gs[j]==0) bm_gs[j]=j-i;
	   j=f[j];
        }
        f[i-1]=--j;
   }

   p=f[0];
   for (j=0;j<=m;++j) {
	if (bm_gs[j]==0) bm_gs[j]=p;
	if (j==p) p=f[p];
   }
   

   for (j=0;j<256;j++) bm_bc[j]=m;
   for (j=0;j<m-1;j++) bm_bc[pattern[j]]=m-j-1;

}

SETL_API void BM_COMPILE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_pat *A; 
STRING_ITERATOR(sb)

char *pattern;                         /* system key                        */
int32 *bm_gs;
int32 *bm_bc;
int m;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","bm_compile");

   m=STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sb,argv[0]);

   pattern = (char *)malloc((size_t)(m+1));
   if (pattern == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sb,pattern);

   A = (struct setl_pat *)(malloc(sizeof(struct setl_pat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   bm_gs = (int32 *)malloc((size_t)(2*sizeof(int32)*(m+1)));
   if (bm_gs == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   bm_bc = (int32 *)malloc((size_t)(sizeof(int32)*256));
   if (bm_bc == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = pat_type+65536*BM_PATTERN;

   A->len = m;
   A->pattern = pattern;
   A->ptr = (void*)bm_gs;
   A->ptr2 = (void*)bm_bc;

   BM_COMPILE_IN(SETL_SYSTEM pattern,m,bm_gs,bm_bc);

   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;

}

SETL_API void BM_EXEC_IN(
  SETL_SYSTEM_PROTO
  char *key,
  int n,
  char *pattern,
  int m,
  int32 *bm_gs,
  int32 *bm_bc,
  specifier *target,
  int free_pattern)
{
int i,j;
TUPLE_CONSTRUCTOR(ca)
int head;

   head = 0;
   i = 0;
   while (i<=n-m) {
   int32 m1,m2;
	for (j=m-1;j>=0 && pattern[j] == key[i+j]; --j);
        if (j<0) {
           specifier s;

           if (!head) {
              TUPLE_CONSTRUCTOR_BEGIN(ca);
              head = 1;
           }
   
           s.sp_form = ft_short;
           s.sp_val.sp_short_value = (i+1);
          
           TUPLE_ADD_CELL(ca,&s);

	   i+=bm_gs[j+1];
        } else {
	   if ((m1=bm_gs[j+1])>(m2=bm_bc[key[i+j]]-m+j+1))
              i+=m1;
           else 
	      i+=m2;
        }
   }

   if (free_pattern) {
      free(pattern);
      free(key);
      free(bm_gs);
      free(bm_bc);
   }

   if (head) {
      TUPLE_CONSTRUCTOR_END(ca);
      unmark_specifier(target);
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
      return;
   }

   unmark_specifier(target);
   target->sp_form = ft_omega;

}

SETL_API void BM_EXEC(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_pat *A; /* w */ 
STRING_ITERATOR(sa)
char *key;                             /* system key                        */
int n;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","bm_exec");

   n=STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sa,argv[0]);

   key = (char *)malloc((size_t)(n+1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,key);

   if ((argv[1].sp_form != ft_opaque)||
       (((argv[1].sp_val.sp_opaque_ptr->type)&65535)!=pat_type))
      abend(SETL_SYSTEM msg_bad_arg,"string matching",2,"bm_exec",
         abend_opnd_str(SETL_SYSTEM argv+1));

   A = (struct setl_pat *)(argv[1].sp_val.sp_opaque_ptr);

   BM_EXEC_IN(SETL_SYSTEM key,n,A->pattern,A->len,(int32*)A->ptr,
	(int32*)A->ptr2,target,NO);

}

SETL_API void BM(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)

char *key;                             /* system key                        */
char *pattern;                         /* system key                        */
int32 *bm_gs;
int32 *bm_bc;
int n,m;


   check_arg(SETL_SYSTEM argv,0,ft_string,"string","bm");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","bm");

   n = STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sa,argv[0]);

   key = (char *)malloc((size_t)(n + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,key);

   m = STRING_LEN(argv[1]);
   ITERATE_STRING_BEGIN(sb,argv[1]);

   pattern = (char *)malloc((size_t)(m + 1));
   if (pattern == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sb,pattern);

   bm_gs = (int32 *)malloc((size_t)(2*sizeof(int32)*(m+1)));
   if (bm_gs == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   bm_bc = (int32 *)malloc((size_t)(sizeof(int32)*256));
   if (bm_bc == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   BM_COMPILE_IN(SETL_SYSTEM pattern,m,bm_gs,bm_bc);
   BM_EXEC_IN(SETL_SYSTEM key,n,pattern,m,bm_gs,bm_bc,target,YES);

}

/*
 *  Edit Distance
 */

SETL_API void EDIST_IN(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target,                  /* return value                      */
  int32 ins,                          /* Score of insert                   */
  int32 del,                          /* Score of delete                   */
  int32 re,                           /* Score of replacement              */
  int32 e)                            /* Score of match                    */
{
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)
TUPLE_ITERATOR(ti)

char *s1,*s2;
int n,m;
int i,j;
int32 *d;
int32 *p,*q;

   n = STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sa,argv[0]);

   s1 = (char *)malloc((size_t)(n + 1));
   if (s1 == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,s1);

   m = STRING_LEN(argv[1]);
   ITERATE_STRING_BEGIN(sb,argv[1]);

   s2 = (char *)malloc((size_t)(m + 1));
   if (s2 == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sb,s2);
 
   d = (int32 *)malloc((size_t)(sizeof(int32)*(n+1)*(m+1)));
   if (d == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);


   /* To avoid using s1[xx-1] and  s2[xx-1] later... */
   s1--; 
   s2--; 

   /* Initialize the first line */

   q=p=d;
   for (j=0;j<=n;j++) *p++=j*del;
 
   i=1; /* Row */


   while (i<=m) {

      /* Initialize the first column */
      *p++=i*ins;

      for (j=1;j<=n;j++) {
	int32 minval;
        int32 s;
        char t;

        minval = *(p-1)+del;
        if ( (s=(*(q+1)+ins))<minval) {
            minval = s;
        }
        if (s1[j]==s2[i]) t=e; else t=re;
        if (( s=(*(q)+t))<minval) {
           minval = s;
        }

        q++;
	*p++=minval;
      }
      i++;
      q++;
   }

   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value =*(p-1);

   s1++;s2++; /* Fix the pointers before freeing */

   free(d);
   free(s1);
   free(s2);

}


SETL_API void ETRANS_IN(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target,                  /* return value                      */
  int32 ins,                          /* Score of insert                   */
  int32 del,                          /* Score of delete                   */
  int32 re,                           /* Score of replacement              */
  int32 e)                            /* Score of match                    */
{
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)
STRING_CONSTRUCTOR(sc)
TUPLE_CONSTRUCTOR(tc)

char *s1,*s2;
int n,m;
int i,j;
int32 *d;
int32 *p,*q;
int32 minval;
char op;
char *r;

specifier s;


   n=STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sa,argv[0]);

   s1 = (char *)malloc((size_t)(n + 1));
   if (s1 == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,s1);

   m=STRING_LEN(argv[1]);
   ITERATE_STRING_BEGIN(sb,argv[1]);

   s2 = (char *)malloc((size_t)(m + 1));
   if (s2 == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sb,s2);
 
   d = (int32 *)malloc((size_t)(sizeof(int32)*(n+1)*(m+1))+1);
   if (d == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);


   /* To avoid using s1[xx-1] and  s2[xx-1] later... */
   s1--; 
   s2--; 

   /* Initialize the first line */

   q=p=d;
   for (j=0;j<=n;j++) { 
       *p++=((j*del)<<2);
   }
 
   i=1; /* Row */


   while (i<=m) {

      /* Op = 1 ^
         Op = 2 \
         Op = 0 <
       */

      /* Initialize the first column */
      *p++=((i*ins)<<2)+1;

      for (j=1;j<=n;j++) {
        int32 s;
        char t;

        minval = (*(p-1)>>2)+del;
        op = 0;
        if ( (s=((*(q+1)>>2)+ins))<minval) {
            minval = s;
            op = 1;
        }
        if (s1[j]==s2[i]) t=e; else t=re;
        if (( s=((*(q)>>2)+t))<minval) {
           minval = s;
           if (t==e) op=2; else op=3;
        }

        q++;
	*p++=(minval<<2)+op;
      }
      i++;
      q++;
   }

   r = (char *)p; /* We'll store the result in the matrix itself */
   *r--=0;
   
   q = p-1; /* Q points to the lower right corner of the table */
   minval = (*q)>>2;


   do {
	op = (*q)&3;
        switch (op)
        {
          case 0:
            q -= 1;
	    *r--='D';
	    break;
          case 1:
            q -= n+1;
	    *r--='I';
            break;
          case 2:
            q -= n+2;
	    *r--='M';
            break;
          case 3:
            q -= n+2;
	    *r--='S';
            break;
        }

   } while (q!=d);

   r++; 
   STRING_CONSTRUCTOR_BEGIN(sc);

   while (*r!=0) {
      STRING_CONSTRUCTOR_ADD(sc,*r);
      r++;
   }


   TUPLE_CONSTRUCTOR_BEGIN(tc);

   s.sp_form = ft_string;
   s.sp_val.sp_string_ptr = STRING_HEADER(sc);

   TUPLE_ADD_CELL(tc,&s);

   s.sp_form = ft_short;
   s.sp_val.sp_short_value = minval;

   TUPLE_ADD_CELL(tc,&s);

   TUPLE_CONSTRUCTOR_END(tc);

   unmark_specifier(target);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(tc);

   s1++;s2++; /* Fix the pointers before freeing */

   free(d);
   free(s1);
   free(s2);

}

SETL_API void EDIST(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","edist");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","edist");

   EDIST_IN(SETL_SYSTEM argc,argv,target,1,1,1,0);


}

SETL_API void EXEDIST(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
TUPLE_ITERATOR(ti)

int32 ins,del,re,e;
int tl;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","exedist");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","exedist");
   check_arg(SETL_SYSTEM argv,2,ft_tuple,"tuple","exedist");

   tl=0;
    
   ITERATE_TUPLE_BEGIN(ti,argv[2]) 
   {
      tl++;
      if ((tl>4)||(ti_element->sp_form!=ft_short)) {
         abend(SETL_SYSTEM "Score Tuple in EDIST must have integer elements");
      }
      switch (tl) {
	case 1:
	   ins = ti_element->sp_val.sp_short_value;
           break;
	case 2:
	   del = ti_element->sp_val.sp_short_value;
           break;
	case 3:
	   re = ti_element->sp_val.sp_short_value;
           break;
	case 4:
	   e = ti_element->sp_val.sp_short_value;
           break;
      }

   }
   ITERATE_TUPLE_END(ti)
   if (tl<4) {
         abend(SETL_SYSTEM "Score Tuple in EDIST must have 4 elements");
      }

   EDIST_IN(SETL_SYSTEM argc,argv,target,ins,del,re,e);
}

SETL_API void ETRANS(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","etrans");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","etrans");

   ETRANS_IN(SETL_SYSTEM argc,argv,target,1,1,1,0);


}

SETL_API void EXETRANS(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
TUPLE_ITERATOR(ti)

int32 ins,del,re,e;
int tl;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","exetrans");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","exetrans");
   check_arg(SETL_SYSTEM argv,2,ft_tuple,"tuple","exetrans");

   tl=0;
    
   ITERATE_TUPLE_BEGIN(ti,argv[2]) 
   {
      tl++;
      if ((tl>4)||(ti_element->sp_form!=ft_short)) {
         abend(SETL_SYSTEM "Score Tuple in EXETRANS must have integer elements");
      }
      switch (tl) {
	case 1:
	   ins = ti_element->sp_val.sp_short_value;
           break;
	case 2:
	   del = ti_element->sp_val.sp_short_value;
           break;
	case 3:
	   re = ti_element->sp_val.sp_short_value;
           break;
	case 4:
	   e = ti_element->sp_val.sp_short_value;
           break;
      }

   }
   ITERATE_TUPLE_END(ti)
   if (tl<4) {
         abend(SETL_SYSTEM "Score Tuple in EXETRANS must have 4 elements");
      }

   ETRANS_IN(SETL_SYSTEM argc,argv,target,ins,del,re,e);
}

SETL_API void AC_COMPILE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
AC_STRUCT *ac;
struct setl_pat *A; 
TUPLE_ITERATOR(ti)
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)
int tuple_el;
int buflen;
char *buffer;

   check_arg(SETL_SYSTEM argv,0,ft_tuple,"tuple","ac_compile");

   A = (struct setl_pat *)(malloc(sizeof(struct setl_pat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   ac = ac_alloc();
   if (ac == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   tuple_el = 0;
   buflen = 0;
   buffer = NULL;
   ITERATE_TUPLE_BEGIN(ti,argv[0]) 
   {
   int i,lb;
   char *p;

       if (ti_element->sp_form!=ft_string) {
          abend(SETL_SYSTEM "Tuple in AC_COMPILE must have string elements");
       }
       lb = STRING_LEN((*ti_element));
       ITERATE_STRING_BEGIN(sb,(*ti_element));

       if (buflen<lb) {
           if (buffer) free(buffer);
           buflen=lb*2;
           buffer = (char *)malloc(buflen+1);
           if (buffer == NULL)
              giveup(SETL_SYSTEM msg_malloc_error);
       }
       p = buffer;
       for (i=0;i<lb;i++) {
         *p++ = ITERATE_STRING_CHAR(sb);
         ITERATE_STRING_NEXT(sb);
       }
       *p=0;
       tuple_el ++;
       if (ac_add_string(ac,buffer,lb,tuple_el)==0)
          abend(SETL_SYSTEM "Error compiling pattern in AC_COMPILE");
       
   }
   ITERATE_TUPLE_END(ti)

   ac_prep(ac);

   if (buffer) free(buffer);

   A->use_count = 1;
   A->type = pat_type+65536*AC_PATTERN;

   A->len = tuple_el;
   A->pattern = NULL;
   A->ptr = (void*)ac;
   A->ptr2 = NULL;

   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;

}

SETL_API void AC_INIT(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_pat *A; /* w */ 
STRING_ITERATOR(sa)
char *key;                             /* system key                        */
int n;
AC_STRUCT *ac;

   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=pat_type))
      abend(SETL_SYSTEM msg_bad_arg,"string matching",1,"ac_init",
         abend_opnd_str(SETL_SYSTEM argv));

   A = (struct setl_pat *)(argv[0].sp_val.sp_opaque_ptr);

   ac = (AC_STRUCT*)(A->ptr);

   check_arg(SETL_SYSTEM argv,1,ft_string,"string","ac_init");

   n = STRING_LEN(argv[1]);
   ITERATE_STRING_BEGIN(sa,argv[1]);

   key = (char *)malloc((size_t)(n + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,key);

   if (ac_search_init(ac,key,n)>0) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = 1;
      return;

   } else {

      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;

   }

}

SETL_API void AC_NEXT_MATCH(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_pat *A; /* w */ 
TUPLE_CONSTRUCTOR(tc)
STRING_ITERATOR(sa)
char *key;                             /* system key                        */
int n;
AC_STRUCT *ac;
int length_out,id_out;
specifier s;

   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=pat_type))
      abend(SETL_SYSTEM msg_bad_arg,"string matching",1,"ac_next_match",
         abend_opnd_str(SETL_SYSTEM argv));

   A = (struct setl_pat *)(argv[0].sp_val.sp_opaque_ptr);

   ac = (AC_STRUCT*)(A->ptr);

   key=ac_search(ac,&length_out, &id_out);

   if (key==NULL) {

      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;

   }

   TUPLE_CONSTRUCTOR_BEGIN(tc);
   
   s.sp_form = ft_short;
   s.sp_val.sp_short_value = id_out;
  
   TUPLE_ADD_CELL(tc,&s);

   s.sp_form = ft_short;
   s.sp_val.sp_short_value = (int)(key-ac->T);

   TUPLE_ADD_CELL(tc,&s);

   s.sp_form = ft_short;
   s.sp_val.sp_short_value = length_out;

   TUPLE_ADD_CELL(tc,&s);

   TUPLE_CONSTRUCTOR_END(tc);

   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(tc);
   return;

}

SETL_API void ST_CREATE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_pat *A; 
TUPLE_ITERATOR(ti)
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)
int tuple_el;
int buflen;
char *buffer;
SUFFIX_TREE st;

  
   A = (struct setl_pat *)(malloc(sizeof(struct setl_pat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   st = stree_new_tree(128,0,LINKED_LIST,0);
   if (st == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);


   A->use_count = 1;
   A->type = pat_type+65536*ST_OBJECT;

   A->len = 0;
   A->pattern = NULL;
   A->ptr = (void*)st;
   A->ptr2 = NULL;

   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;

}


SETL_API void ST_ADD_STRING(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_pat *A; 
STRING_ITERATOR(sa)
int i;
int n;
char *key;
SUFFIX_TREE st;

   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=pat_type))
      abend(SETL_SYSTEM msg_bad_arg,"suffix tree",1,"st_add_string",
         abend_opnd_str(SETL_SYSTEM argv));

   check_arg(SETL_SYSTEM argv,1,ft_string,"string","st_match");
   
   n = STRING_LEN(argv[1]);
   ITERATE_STRING_BEGIN(sa,argv[1]);

   key = (char *)malloc((size_t)(n + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,key);


   A = (struct setl_pat *)(argv[0].sp_val.sp_opaque_ptr);

   st = (SUFFIX_TREE)(A->ptr);

 
   i=stree_ukkonen_add_string(st,(char *)key,(char*)key,n,A->len+1);
 
   
   if (i==0) {
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;
       
   } 
    A->len++;
   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value =A->len;



}


static MATCHES matchlist;
static int matchcount, matcherror, patlen;

static int add_match(SUFFIX_TREE tree, STREE_NODE node)
{
  int i, pos, id;
  char *seq;
  MATCHES newmatch;
  
  for (i=1; stree_get_leaf(tree, node, i, &seq, &pos, &id); i++) {
    newmatch = alloc_match();
    if (newmatch == NULL) {
      free_matches(matchlist);
      matchlist = NULL;
      matcherror = 1;
      return 0;
    }

    /*
     * Shift positions by 1 here (from 0..N-1 to 1..N).
     */
    newmatch->type = TEXT_SET_EXACT;
    newmatch->lend = pos + 1;
    newmatch->rend = pos + patlen;
    newmatch->textid = id;

	  newmatch->next = matchlist;
    matchlist = newmatch;
    matchcount++;
  }

  return 1;
}


SETL_API void ST_MATCH(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_pat *A; /* w */ 
TUPLE_CONSTRUCTOR(ta)
TUPLE_CONSTRUCTOR(tc)
STRING_ITERATOR(sa)
char *key;                             /* system key                        */
int n;
SUFFIX_TREE st;
int length_out,pos;
int matchcount,matcherror = 0;
STREE_NODE node;
int i,id;
char *seq;
specifier s;
MATCHES ptr;

   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=pat_type))
      abend(SETL_SYSTEM msg_bad_arg,"string matching",1,"st_match",
         abend_opnd_str(SETL_SYSTEM argv));

   check_arg(SETL_SYSTEM argv,1,ft_string,"string","st_match");

   n = STRING_LEN(argv[1]);
   ITERATE_STRING_BEGIN(sa,argv[1]);

   key = (char *)malloc((size_t)(n + 1));
   if (key == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,key);


   A = (struct setl_pat *)(argv[0].sp_val.sp_opaque_ptr);

   st = (SUFFIX_TREE)(A->ptr);

   length_out = stree_match(st,key,n,&node,&pos);
   free(key);

   if (length_out < n) {

      unmark_specifier(target);
      target->sp_form = ft_omega;
      return;

   }



  /*
   * Traverse the subtree, finding the matches.
   */
  matchlist = NULL;
  matchcount = matcherror = 0;
  patlen=n;
 
  stree_traverse_subtree(st, node, add_match, (int (*)()) NULL);
  if (matcherror) {
	  unmark_specifier(target);
	  target->sp_form = ft_omega;
	  return;
  }
    
    
#ifdef SORT_MATCHES  
    /*
     * Bubble sort the matches.
     */
    flag = 1;
    while (flag) {
      flag = 0;
      back = NULL;
      current = matchlist;
      while (current->next != NULL) {
        if (current->next->textid < current->textid ||
            (current->next->textid == current->textid &&
             current->next->lend < current->lend)) {
          /*
           * Move current->next before current in the list.
           */
          next = current->next;
          current->next = next->next;
          next->next = current;
          if (back == NULL)
            back = matchlist = next;
          else
            back = back->next = next;
          
          flag = 1;
        }
        else {
          back = current;
          current = current->next;
        }
      }
    }
#endif
  

   TUPLE_CONSTRUCTOR_BEGIN(ta);
   
	 
	   
	   ptr=matchlist;
	   while (ptr!=NULL) {
	   
	       TUPLE_CONSTRUCTOR_BEGIN(tc);
	        
	   	   s.sp_form = ft_short;
		   s.sp_val.sp_short_value = ptr->textid;
		  
		   TUPLE_ADD_CELL(tc,&s);

		   s.sp_form = ft_short;
		   s.sp_val.sp_short_value = (int)(ptr->lend);

		   TUPLE_ADD_CELL(tc,&s);

		   s.sp_form = ft_short;
		   s.sp_val.sp_short_value = (int)(ptr->rend);

		   TUPLE_ADD_CELL(tc,&s);
		   
		   TUPLE_CONSTRUCTOR_END(tc);
				   
		   s.sp_form = ft_tuple;
		   s.sp_val.sp_tuple_ptr = TUPLE_HEADER(tc);
		   TUPLE_ADD_CELL(ta,&s);
		   
		   ptr=ptr->next;
	   }
	 

   TUPLE_CONSTRUCTOR_END(ta);

   free_matches(matchlist);
    
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ta);
   return;

}
SETL_API void PWSCORES(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_pat *A; 
TUPLE_ITERATOR(ti)
TUPLE_ITERATOR(t2)
STRING_ITERATOR(sa)
int tuple_el;
int buflen;
char *buffer;

   check_arg(SETL_SYSTEM argv,0,ft_tuple,"tuple","pwscores");

   A = (struct setl_pat *)(malloc(sizeof(struct setl_pat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   buffer = (char*)malloc(65536);
   if (buffer == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   bzero(buffer,65536);

   tuple_el = 0;
   ITERATE_TUPLE_BEGIN(ti,argv[0]) 
   {
   int i;
   char a,b;

       if (ti_element->sp_form!=ft_tuple) {
          abend(SETL_SYSTEM "Tuple in PWSCORES must have tuple elements");
       }
       tuple_el = 0;
       ITERATE_TUPLE_BEGIN(t2,(*ti_element));
       {
          switch (tuple_el) 
          {
              case 0:
                 if (t2_element->sp_form==ft_short) {
                   if (t2_element->sp_val.sp_short_value!=0) 
                      abend(SETL_SYSTEM "The first component in tuple "
                           "elements in PWSCORES must be a character or 0");
                   else {
                      a = 0;
                      break;
                   }
                 }
                 if (t2_element->sp_form!=ft_string) {
                      abend(SETL_SYSTEM "The first component in tuple "
                           "elements in PWSCORES must be a character or 0");
                 }
                 ITERATE_STRING_BEGIN(sa,(*t2_element));
                 a = ITERATE_STRING_CHAR(sa);
                 break;
              case 1:
                 if (t2_element->sp_form==ft_short) {
                   if (t2_element->sp_val.sp_short_value!=0) 
                      abend(SETL_SYSTEM "The second component in tuple "
                           "elements in PWSCORES must be a character or 0");
                   else {
                      b = 0;
                      break;
                   }
                 }
                 if (t2_element->sp_form!=ft_string) {
                      abend(SETL_SYSTEM "The second component in tuple "
                           "elements in PWSCORES must be a character or 0");
                 }
                 ITERATE_STRING_BEGIN(sa,(*t2_element));
                 b = ITERATE_STRING_CHAR(sa);
                 break;
              case 2:
                 if (t2_element->sp_form!=ft_short) {
                      abend(SETL_SYSTEM "The third component in tuple "
                           "elements in PWSCORES must be an integer");
                 }
                 if ((t2_element->sp_val.sp_short_value>127)||
                     (t2_element->sp_val.sp_short_value<-127)) {
                      abend(SETL_SYSTEM 
                           "The weights in PWSCORES must be in [-127,127]");
                 }
                 buffer[a*256+b]=t2_element->sp_val.sp_short_value;
                 buffer[b*256+a]=t2_element->sp_val.sp_short_value;
                 break;
          }
 
       
          tuple_el ++;
          if (tuple_el>3) {
             abend(SETL_SYSTEM 
                  "The tuple elements in PWSCORES must have length 3");
          }
       
       }
       ITERATE_TUPLE_END(t2)
       if (tuple_el<3) {
             abend(SETL_SYSTEM 
                  "The tuple elements in PWSCORES must have length 3");
          }
   }
   ITERATE_TUPLE_END(ti)


   A->use_count = 1;
   A->type = pat_type+65536*PW_SCORES;

   A->len = 0;
   A->pattern = NULL;
   A->ptr = (void*)buffer;
   A->ptr2 = NULL;

   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;

}

/*
 *      Longest common subsequence
 */

SETL_API void LCSEQ(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)
STRING_CONSTRUCTOR(sc)
TUPLE_CONSTRUCTOR(tc)

char *s1,*s2,*r;
int n,m;
int i,j;
int32 *d;
int32 *p,*q;
int op;
int32 maxval;
specifier s;

   n = STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sa,argv[0]);

   s1 = (char *)malloc((size_t)(n + 1));
   if (s1 == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,s1);

   m = STRING_LEN(argv[1]);
   ITERATE_STRING_BEGIN(sb,argv[1]);

   s2 = (char *)malloc((size_t)(m + 1));
   if (s2 == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sb,s2);

   d = (int32 *)malloc((size_t)(sizeof(int32)*(n+1)*(m+1))+1);
   if (d == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);


   /* To avoid using s1[xx-1] and  s2[xx-1] later... */
   s1--; 
   s2--; 

   /* Initialize the first line */

   q=p=d;

   for (j=0;j<=n;j++) {
       *p++=0;
   }
 
   i=1; /* Row */


   while (i<=m) {

      /* Op = 1 ^
         Op = 2 \
         Op = 0 <
       */

      /* Initialize the first column */
      *p++=1; /* It's really (0<<2)+1 to encode the operation */

      for (j=1;j<=n;j++) {
        int32 s;
        char t=0;


        maxval = *(p-1)>>2; /* i,j-1 */
        op = 0;
        if ( (s=(*(q+1))>>2)>maxval) { /*i-1,j*/
            maxval = s;
            op = 1;
        }
        if (s2[i]==s1[j]) t=1;         
        if (( s=((*(q)>>2)+t))>maxval) { /*i-1,j-1*/
           maxval = s;
           op = 3-t;
        }

        q++;
	*p++=(maxval<<2)+op;
      }
      i++;
      q++;
   }

   r = (char *)p; /* We'll store the result in the matrix itself */
   *r--=0;
   
   q = p-1; /* Q points to the lower right corner of the table */
   maxval = (*q)>>2;


   do {
	op = (*q)&3;
        switch (op)
        {
          case 0:
            q -= 1;
	    break;
          case 1:
            q -= n+1;
            break;
          case 2:
            /* Get i from q */
            i = (q-d)/(n+1);
	    *r--=s2[i];
            q -= n+2;
            break;
          case 3:
            q -= n+2;
            break;
        }

   } while (q!=d);

   r++; 
   STRING_CONSTRUCTOR_BEGIN(sc);

   while (*r!=0) {
      STRING_CONSTRUCTOR_ADD(sc,*r);
      r++;
   }


   TUPLE_CONSTRUCTOR_BEGIN(tc);

   s.sp_form = ft_string;
   s.sp_val.sp_string_ptr = STRING_HEADER(sc);

   TUPLE_ADD_CELL(tc,&s);

   s.sp_form = ft_short;
   s.sp_val.sp_short_value = maxval;

   TUPLE_ADD_CELL(tc,&s);

   TUPLE_CONSTRUCTOR_END(tc);

   unmark_specifier(target);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(tc);

   s1++;s2++; /* Fix the pointers before freeing */

   free(d);
   free(s1);
   free(s2);

}
/*
 *  String Similarity
 */

SETL_API void SIMIL(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)
struct setl_pat *A;

char *pwscores;
char *s1,*s2;
int n,m;
int i,j;
int32 *d;
int32 *p,*q;
int sum;

   n = STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sa,argv[0]);

   s1 = (char *)malloc((size_t)(n + 1));
   if (s1 == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,s1);

   m = STRING_LEN(argv[1]);
   ITERATE_STRING_BEGIN(sb,argv[1]);

   s2 = (char *)malloc((size_t)(m + 1));
   if (s2 == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sb,s2);

   if ((argv[2].sp_form != ft_opaque)||
       (((argv[2].sp_val.sp_opaque_ptr->type)&65535)!=pat_type))
      abend(SETL_SYSTEM msg_bad_arg,"string matching",3,"simil",
         abend_opnd_str(SETL_SYSTEM argv+2));

   A = (struct setl_pat *)(argv[2].sp_val.sp_opaque_ptr);
  
   pwscores=A->ptr;
 
   d = (int32 *)malloc((size_t)(sizeof(int32)*(n+1)*(m+1)));
   if (d == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);


   /* To avoid using s1[xx-1] and  s2[xx-1] later... */
   s1--; 
   s2--; 

   /* Initialize the first line */

   q=p=d;

   sum = 0;
   for (j=0;j<=n;j++) {
       *p++=sum;
       if (j<n) sum+=pwscores[s1[j+1]];
   }
 
   i=1; /* Row */


   sum = 0;
   while (i<=m) {

      /* Initialize the first column */
      sum+=pwscores[s2[i]];
      *p++=sum;

      for (j=1;j<=n;j++) {
	int32 maxval;
        int32 s;
        char t;

        t = s2[i];

        maxval = *(p-1)+pwscores[s1[j]];; /* i,j-1 */
        if ( (s=(*(q+1)+pwscores[t]))>maxval) { /*i-1,j*/
            maxval = s;
        }
        if (( s=(*(q)+pwscores[t*256+s1[j]]))>maxval) { /*i-1,j-1*/
           maxval = s;
        }

        q++;
	*p++=maxval;
      }
      i++;
      q++;
   }

   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value =*(p-1);

   s1++;s2++; /* Fix the pointers before freeing */

   free(d);
   free(s1);
   free(s2);

}

SETL_API void SIMILT(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)
struct setl_pat *A;
STRING_CONSTRUCTOR(sc)
TUPLE_CONSTRUCTOR(tc)

char *pwscores;
char *s1,*s2,*r;
int n,m;
int i,j;
int32 *d;
int32 *p,*q;
int sum;
int32 maxval;
char op;
specifier s;

   n = STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sa,argv[0]);

   s1 = (char *)malloc((size_t)(n + 1));
   if (s1 == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sa,s1);

   m = STRING_LEN(argv[1]);
   ITERATE_STRING_BEGIN(sb,argv[1]);

   s2 = (char *)malloc((size_t)(m + 1));
   if (s2 == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   STRING_CONVERT(sb,s2);

   if ((argv[2].sp_form != ft_opaque)||
       (((argv[2].sp_val.sp_opaque_ptr->type)&65535)!=pat_type))
      abend(SETL_SYSTEM msg_bad_arg,"string matching",3,"simil",
         abend_opnd_str(SETL_SYSTEM argv+2));

   A = (struct setl_pat *)(argv[2].sp_val.sp_opaque_ptr);
  
   pwscores=A->ptr;
 
   d = (int32 *)malloc((size_t)(sizeof(int32)*(n+1)*(m+1))+1);
   if (d == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);


   /* To avoid using s1[xx-1] and  s2[xx-1] later... */
   s1--; 
   s2--; 

   /* Initialize the first line */

   q=p=d;

   sum = 0;
   for (j=0;j<=n;j++) {
       *p++=(sum<<2);
       if (j<n) sum+=pwscores[s1[j+1]];
   }
 
   i=1; /* Row */


   sum = 0;
   while (i<=m) {

      /* Op = 1 ^
         Op = 2 \
         Op = 0 <
       */

      /* Initialize the first column */
      sum+=pwscores[s2[i]];
      *p++=(sum<<2)+1;

      for (j=1;j<=n;j++) {
        int32 s;
        char t;

        t = s2[i];

        maxval = (*(p-1)>>2)+pwscores[s1[j]];; /* i,j-1 */
        op = 0;
        if ( (s=((*(q+1)>>2)+pwscores[t]))>maxval) { /*i-1,j*/
            maxval = s;
            op = 1;
        }
        if (( s=((*(q)>>2)+pwscores[t*256+s1[j]]))>maxval) { /*i-1,j-1*/
           maxval = s;
           op = 2;
        }

        q++;
        *p++=(maxval<<2)+op;
      }
      i++;
      q++;
   }

   r = (char *)p; /* We'll store the result in the matrix itself */
   *r--=0;
   
   q = p-1; /* Q points to the lower right corner of the table */
   maxval = (*q)>>2;

   do {
	op = (*q)&3;
        switch (op)
        {
          case 0:
            q -= 1;
	    *r--='D';
	    break;
          case 1:
            q -= n+1;
	    *r--='I';
            break;
          case 2:
            q -= n+2;
	    *r--='A';
            break;
        }

   } while (q!=d);

   r++; 
   STRING_CONSTRUCTOR_BEGIN(sc);

   while (*r!=0) {
      STRING_CONSTRUCTOR_ADD(sc,*r);
      r++;
   }


   TUPLE_CONSTRUCTOR_BEGIN(tc);

   s.sp_form = ft_string;
   s.sp_val.sp_string_ptr = STRING_HEADER(sc);

   TUPLE_ADD_CELL(tc,&s);

   s.sp_form = ft_short;
   s.sp_val.sp_short_value = maxval;

   TUPLE_ADD_CELL(tc,&s);

   TUPLE_CONSTRUCTOR_END(tc);

   unmark_specifier(target);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(tc);

   s1++;s2++; /* Fix the pointers before freeing */

   free(d);
   free(s1);
   free(s2);

}
