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
 *  This is a String Utility Native Package for SETL2
 *
 */



/* SETL2 system header files */
                     
#include "macros.h"

#define YES 1
#define NO  0



struct setl_flat {                     /* Native Object Structure           */
   int32 use_count;                    /* Reference Count                   */
   int32 type;                         /* Encodes Type and Subtype          */
   int32 len;                          
   char *str;
};


#define FLAT_TYPE         1            /* Flat String Setl Object           */
 
static int32 str_type;                 /* Store the type assigned to us by  */
                                       /* the SETL runtime                  */

 static void internal_destructor(struct setl_flat *spec)
{
int subtyp;

    if ((spec!=NULL)&&((spec->type&65535)==str_type)) {
    
           if (spec->str!=NULL) {
#ifdef MEMDEBUG
			    printf("#");
#endif		
				free(spec->str);
           		spec->str=NULL;
           		}
	   free(spec);
        }
    

}


SETL_API int32 STRING_UTILITY_PAK__INIT(
   SETL_SYSTEM_PROTO_VOID
   )
{
   str_type=register_type(SETL_SYSTEM "string utilities",internal_destructor);
   
   if (str_type==0) return 1;
   return 0;

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

int check_int(
  SETL_SYSTEM_PROTO
  specifier *argv,                  
  int param,
  int type,
  char *typestr,
  char *routine)
{

 if (argv[param].sp_form == ft_short) 
      return (int)(argv[param].sp_val.sp_short_value);
   else if (argv[param].sp_form == ft_long) 
      return (int)long_to_short(SETL_SYSTEM argv[param].sp_val.sp_long_ptr);
   else 
      abend(SETL_SYSTEM msg_bad_arg,"integer",param+1,routine,
            abend_opnd_str(SETL_SYSTEM argv+param));
       
}


SETL_API  void FLAT_CREATE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_flat *A; /* w */ 
int len;

   len = check_int(SETL_SYSTEM argv,0,ft_short,"integer","flat_create");

   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
  if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = str_type;
   A->len=len;
   
   A->str = (char *)malloc(len+1);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   #ifdef MEMDEBUG
   printf("*");
#endif

   bzero(A->str,len);

   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;

}

SETL_API void FLAT_CLONE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_flat *S;  
struct setl_flat *A;  
int len; 


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat clone",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   S = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   len=S->len;
   
   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = str_type;
   A->len=len;
   
   A->str = (char *)malloc(len+1);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
  #ifdef MEMDEBUG
   printf("*");
#endif

   memcpy(A->str,S->str,len+1);

   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
   
   return;

}

SETL_API void FLAT_SLICE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S;  
struct setl_flat *A;  
int len;
int s1,s2; 


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat slice",
         abend_opnd_str(SETL_SYSTEM argv+0));

   s1=check_int(SETL_SYSTEM argv,1,ft_short,"integer","flat_slice");
   s2=check_int(SETL_SYSTEM argv,2,ft_short,"integer","flat_slice");
   
  


   S = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   len=S->len;
   
 
   if ((s1<1)||(s2<1)||(s1>s2)||(s1>len)||(s2>len))
		abend(SETL_SYSTEM "the slice parameters (%d,%d) in FLAT_SLICE are out of range (1,%d)\n",s1,s2,len);
  
   
   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = str_type;
   A->len=s2-s1+1;
   
   A->str = (char *)malloc(A->len+1);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   #ifdef MEMDEBUG
   printf("*");
#endif


   memcpy(A->str,S->str+s1-1,A->len);
   A->str[A->len]=0;
   
   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
   
   return;

}

SETL_API void FLAT_SLICE_END(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S;  
struct setl_flat *A;  
int len;
int s1; 


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat_slice_end",
         abend_opnd_str(SETL_SYSTEM argv+0));

   s1=check_int(SETL_SYSTEM argv,1,ft_short,"integer","flat_slice_end");

   S = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   len=S->len;
   
  
  
    if ((s1<1)||(s1>len))
		abend(SETL_SYSTEM "the slice parameter (%d) in FLAT_SLICE_END is out of range (1,%d)\n",s1,len);
  
   
   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = str_type;
   A->len=len-s1+1;
   
   A->str = (char *)malloc(A->len+1);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
#ifdef MEMDEBUG
   printf("*");
#endif
   memcpy(A->str,S->str+s1-1,len-s1+2);
 
   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
   
   return;

}


SETL_API void FLAT_REVERSE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S;  
struct setl_flat *A;  
int len; 
char *s,*d;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat len",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   S = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   len=S->len;
   
   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = str_type;
   A->len=len;
   
   A->str = (char *)malloc(len+1);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
#ifdef MEMDEBUG
   printf("*");
#endif

   d=A->str+len;
   s=S->str;
   *d--=0;
   while (len>0) {
      *d--=*s++;len--;
   }

   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
   
   return;

}

SETL_API void FLAT_REVERSE_TRANSLATE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S;   
struct setl_flat *A;  
int len; 
char *s,*d,*m;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat reverse translate",
         abend_opnd_str(SETL_SYSTEM argv+0));

 
   if ((argv[1].sp_form != ft_opaque)||
       (((argv[1].sp_val.sp_opaque_ptr->type)&65535)!=str_type)||
       ((struct setl_flat*)(argv[1].sp_val.sp_opaque_ptr))->len!=256)
       abend(SETL_SYSTEM msg_bad_arg,"flat string(256)",2,"flat reverse translate",
         abend_opnd_str(SETL_SYSTEM argv+1));

 

   S = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   len=S->len;
   m = ((struct setl_flat *)(argv[1].sp_val.sp_opaque_ptr))->str;
   
   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = str_type;
   A->len=len;
   
   A->str = (char *)malloc(len+1);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
#ifdef MEMDEBUG
   printf("*");
#endif
  
   d=A->str;
   s=S->str+len-1;
   
   while (len>0) {
      char tc=m[*s--];
      if (tc!=0) *d++=tc; else A->len--;
      len--;
   }
   *d=0;
   
   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
   
   return;

}

SETL_API void FLAT_TRANSLATE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S;   
struct setl_flat *A;  
int len; 
char *s,*d,*m;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat translate",
         abend_opnd_str(SETL_SYSTEM argv+0));

 
   if ((argv[1].sp_form != ft_opaque)||
       (((argv[1].sp_val.sp_opaque_ptr->type)&65535)!=str_type)||
       ((struct setl_flat*)(argv[1].sp_val.sp_opaque_ptr))->len!=256)
       abend(SETL_SYSTEM msg_bad_arg,"flat string(256)",2,"flat translate",
         abend_opnd_str(SETL_SYSTEM argv+1));

 

   S = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   len=S->len;
   m = ((struct setl_flat *)(argv[1].sp_val.sp_opaque_ptr))->str;
   
   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = str_type;
   A->len=len;
   
   A->str = (char *)malloc(len+1);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
  #ifdef MEMDEBUG
   printf("*");
#endif

   d=A->str;
   s=S->str;
   while (len>0) {
  	 char tc=m[*s++];
      if (tc!=0) *d++=tc; else (A->len)--;
      len--;
     
   }
   *d=0;
   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
   
   return;

}
SETL_API void FLAT_LEN(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *A; /* w */ 
int32 len;
int32 hi_bits;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat len",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   A = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   len=A->len;

   unmark_specifier(target);

   if (!(!(hi_bits = len & INT_HIGH_BITS) ||
                        hi_bits == INT_HIGH_BITS)) {
		
			target->sp_form = ft_omega;
			short_to_long(SETL_SYSTEM target,len);

   } else { 

	    target->sp_form = ft_short;
		target->sp_val.sp_short_value = len;
	
   }
   
   

   return;

}

SETL_API void FLAT_TO_SETL(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_CONSTRUCTOR(cs)
struct setl_flat *A; /* w */ 
int len;
int i;
char *c;

   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat_to_setl",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   A = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);

   STRING_CONSTRUCTOR_BEGIN(cs);

   i=0;
   c=(char *)A->str;
   while (i<A->len) {
      
        STRING_CONSTRUCTOR_ADD(cs,*c++);
        i++;
   }

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = STRING_HEADER(cs);
   return;

}

SETL_API void FLAT_FROM_SETL(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_flat *A; /* w */ 
STRING_ITERATOR(sc)
int i,len;
unsigned char *str;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","flat_from_setl");
 
   len = STRING_LEN(argv[0]);
   ITERATE_STRING_BEGIN(sc,argv[0]);
   

   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = str_type;
   A->len=len;
   
   A->str = (char *)malloc(len+1);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

#ifdef MEMDEBUG
   printf("*");
#endif

 
   ITERATE_STRING_BEGIN(sc,argv[0]);
   str=A->str;
   for (i=0;i<len;i++) {
      *str++=(unsigned char)ITERATE_STRING_CHAR(sc);
      ITERATE_STRING_NEXT(sc);
   }
   *str=0;
 
   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
 
   return;
   

}


SETL_API void FLAT_GET_CHAR(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_CONSTRUCTOR(cs)
struct setl_flat *A; /* w */ 

int j;
char *c;

   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat_get_char",
         abend_opnd_str(SETL_SYSTEM argv+0));

   j=check_int(SETL_SYSTEM argv,1,ft_short,"integer","flat_get_char");

   
   A = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   
 
  
   if ((j<1)||(j>A->len))
		abend(SETL_SYSTEM "the slice parameter (%d) in FLAT_GET_CHAR is out of range (1,%d)\n",j,A->len);


   STRING_CONSTRUCTOR_BEGIN(cs);
     
   STRING_CONSTRUCTOR_ADD(cs,*(A->str+j-1));
  

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = STRING_HEADER(cs);
   return;

}

SETL_API void FLAT_TRANSLATE_ALL(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
TUPLE_CONSTRUCTOR(ca)
TUPLE_CONSTRUCTOR(cb)
STRING_CONSTRUCTOR(sa)
TUPLE_ITERATOR(ti)
int i;
int j;
int rarity[65];
int offset,minlen;
int tuple_el;
struct setl_flat *A; /* w */ 
struct setl_flat *CODE; /* w */ 
char *tr_buffer=NULL;
int tr_max=4096;
unsigned char *str;
unsigned char map[256];
int score;
char *codestr;
int z;
char *d;
int start;
specifier s;

   /*/flat_translate_all(obj,off,minlen,codestr,rar); -- */
 
   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat_translate_all",
         abend_opnd_str(SETL_SYSTEM argv+0));
    
 
   if ((argv[3].sp_form != ft_opaque)||
       (((argv[3].sp_val.sp_opaque_ptr->type)&65535)!=str_type)||
       ((struct setl_flat*)(argv[3].sp_val.sp_opaque_ptr))->len!=65)
       abend(SETL_SYSTEM msg_bad_arg,"flat string(65)",2,"flat_translate_all",
         abend_opnd_str(SETL_SYSTEM argv+3));

         
   offset=check_int(SETL_SYSTEM argv,1,ft_short,"integer","flat_translate_all")-1;
   minlen=check_int(SETL_SYSTEM argv,2,ft_short,"integer","flat_translate_all");
   check_arg(SETL_SYSTEM argv,4,ft_tuple,"tuple","flat_translate_all");
   
   A = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   CODE = (struct setl_flat *)(argv[3].sp_val.sp_opaque_ptr);
   codestr=CODE->str;
  
  
   if (TUPLE_LEN(argv[4])!=65)
   		giveup(SETL_SYSTEM "The rarity tuple must have len 65");
   	
   	tuple_el=0;	
   ITERATE_TUPLE_BEGIN(ti,argv[4]) 
   {

       if (ti_element->sp_form!=ft_short) {
          abend(SETL_SYSTEM "Tuple in FLAT_TRANSLATE_ALL must have short elements");
       }
       rarity[tuple_el++]=ti_element->sp_val.sp_short_value;
     
     
   }
   ITERATE_TUPLE_END(ti)
 
   tr_buffer=(char*)malloc(tr_max+1);
   if (tr_buffer==NULL)
   		giveup(SETL_SYSTEM msg_malloc_error);
 

   TUPLE_CONSTRUCTOR_BEGIN(ca);

	for (i=0;i<256;i++) 
		map[i]=64;
		
	
	map['A']=map['a']=0;
	map['C']=map['c']=1;
	map['T']=map['t']=2;
	map['G']=map['g']=3;
	
	



	i=offset;
	j=0;
	str=A->str+i;
	score=0;
	d=tr_buffer;
	start=i;
	while (i+3<=A->len) {
		
		int k;
		unsigned char tr;
		
		k=map[*str]+map[*(str+1)]*4+map[*(str+2)]*16;
		if (k>63) k=64;
		str+=3;
		
	
		
		if (j>=tr_max) {
			tr_max*=2;
			tr_buffer=(char*)realloc(tr_buffer,tr_max+1);
		}
		
		tr=codestr[k];
		if (tr==255) {
			
			if (j>=minlen) {
			
           
           
	   		   TUPLE_CONSTRUCTOR_BEGIN(cb);
	   
	   		
	   		   s.sp_form = ft_short;
		       s.sp_val.sp_short_value = start+1;	   
	           TUPLE_ADD_CELL(cb,&s);
	           
	           
	   		   s.sp_form = ft_short;
		       s.sp_val.sp_short_value = j;   
	           TUPLE_ADD_CELL(cb,&s);
	           
	         
	         
	            STRING_CONSTRUCTOR_BEGIN(sa);
				
			   z=0;
     		   while (z<j) {
            		 STRING_CONSTRUCTOR_ADD(sa,tr_buffer[z]);
	 				 z++;
               }

			   s.sp_form=ft_string;
       		   s.sp_val.sp_string_ptr = STRING_HEADER(sa);
       		   TUPLE_ADD_CELL(cb,&s);
             
	           
	   	       s.sp_form = ft_short;
	           s.sp_val.sp_short_value=score;
	           TUPLE_ADD_CELL(cb,&s);
	          
	           TUPLE_CONSTRUCTOR_END(cb);
		           
  	           s.sp_form=ft_tuple;
		       s.sp_val.sp_tuple_ptr =TUPLE_HEADER(cb);
		       TUPLE_ADD_CELL(ca,&s);
		    
			}
			start=i+3;
			score=0;
			j=0;
			d=tr_buffer;
			
			
		} else {
		
			*d++=tr;
			j++;
			score+=rarity[k];
		}		
		i+=3;
	}
	
	 if (j>=minlen) {
			
			
           
	   		   TUPLE_CONSTRUCTOR_BEGIN(cb);
	   
	   		   s.sp_form = ft_short;
	           s.sp_val.sp_short_value=start+1;
	           TUPLE_ADD_CELL(cb,&s);
	        
	   		   s.sp_form = ft_short;
	           s.sp_val.sp_short_value=j;
	           TUPLE_ADD_CELL(cb,&s);
	        
	           
	           STRING_CONSTRUCTOR_BEGIN(sa);
				
			   z=0;
     		   while (z<j) {
            		 STRING_CONSTRUCTOR_ADD(sa,tr_buffer[z]);
	 				 z++;
               }

			   s.sp_form=ft_string;
       		   s.sp_val.sp_string_ptr = STRING_HEADER(sa);
               TUPLE_ADD_CELL(cb,&s);
               
	   		   s.sp_form = ft_short;
	           s.sp_val.sp_short_value=score;
	           TUPLE_ADD_CELL(cb,&s);
	          
	           TUPLE_CONSTRUCTOR_END(cb);
		           
  	           s.sp_form=ft_tuple;
		       s.sp_val.sp_tuple_ptr =TUPLE_HEADER(cb);
		       TUPLE_ADD_CELL(cb,&s);
		    
			}
  
   		


   free(tr_buffer);
   

   TUPLE_CONSTRUCTOR_END(ca);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
   return;



}




SETL_API void FLAT_SET_CHAR(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_CONSTRUCTOR(cs)
STRING_ITERATOR(sc)

struct setl_flat *A; /* w */ 
int j,len;
char *c;

   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat_set_char",
         abend_opnd_str(SETL_SYSTEM argv+0));

   j=check_int(SETL_SYSTEM argv,1,ft_short,"integer","flat_set_char");
   check_arg(SETL_SYSTEM argv,2,ft_string,"char","flat_set_char");
   
   A = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
  

   if ((j<1)||(j>A->len))
		abend(SETL_SYSTEM "the slice parameter (%d) in FLAT_SET_CHAR is out of range (1,%d)\n",j,A->len);
   
   ITERATE_STRING_BEGIN(sc,argv[2]);
   
   len = STRING_LEN(argv[2]);
   if (len!=1)
    	giveup(SETL_SYSTEM "the setl string must be a char");
    

   A->str[j-1]=(unsigned char)ITERATE_STRING_CHAR(sc);
  
  

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

SETL_API void FLAT_ADD(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S1;   
struct setl_flat *S2; 
struct setl_flat *A;   
char *s,*d;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat add",
         abend_opnd_str(SETL_SYSTEM argv+0));

 
   if ((argv[1].sp_form != ft_opaque)||
       (((argv[1].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",2,"flat add",
         abend_opnd_str(SETL_SYSTEM argv+1));

 

   S1 = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   S2 = (struct setl_flat *)(argv[1].sp_val.sp_opaque_ptr);
   
   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = str_type;
   A->len=S1->len+S2->len;
   
   A->str = (char *)malloc(A->len+1);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
  #ifdef MEMDEBUG
   printf("*");
#endif

   memcpy(A->str,S1->str,S1->len);
   memcpy(A->str+S1->len,S2->str,S2->len+1);

   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
   
   return;

}

SETL_API void FLAT_MATCH_SCORES(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S1;   
struct setl_flat *S2; 
int offset,repeats;
TUPLE_CONSTRUCTOR(ca)
char *s1,*s2;
specifier s;
int i;

   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat add",
         abend_opnd_str(SETL_SYSTEM argv+0));

 
   if ((argv[1].sp_form != ft_opaque)||
       (((argv[1].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",2,"flat add",
         abend_opnd_str(SETL_SYSTEM argv+1));

   offset=check_int(SETL_SYSTEM argv,2,ft_short,"integer","FLAT_MATCH_SCORES")-1;
   repeats=check_int(SETL_SYSTEM argv,3,ft_short,"integer","FLAT_MATCH_SCORES");
 
   
   S1 = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   S2 = (struct setl_flat *)(argv[1].sp_val.sp_opaque_ptr);
  
   if ((S1->len-(repeats+offset-1))<(S2->len))
   		giveup(SETL_SYSTEM "Repeats in FLAT_MATCH_SCORES out of range");
   
   TUPLE_CONSTRUCTOR_BEGIN(ca);

   for (i=0;i<repeats;i++) {
   int m=0;
   char *end;
    
   	s1=S1->str+offset;
   	s2=S2->str;
   	end=s2+S2->len;
   	while (s2<end) {
   	   if (*s2++==*s1++) m++;
   	}
   

	s.sp_form = ft_short;
	s.sp_val.sp_short_value = m;
	TUPLE_ADD_CELL(ca,&s);

     offset++;
   }
 
   TUPLE_CONSTRUCTOR_END(ca);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
   return;

}



SETL_API void FLAT_SET_SLICE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S1;   
struct setl_flat *S2; 
int i;
char *s,*d;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat_set_slice",
         abend_opnd_str(SETL_SYSTEM argv+0));

 
   if ((argv[2].sp_form != ft_opaque)||
       (((argv[2].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",3,"flat_set_slice",
         abend_opnd_str(SETL_SYSTEM argv+2));

   i=check_int(SETL_SYSTEM argv,1,ft_short,"integer","flat_set_slice");

   S1 = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   S2 = (struct setl_flat *)(argv[2].sp_val.sp_opaque_ptr);
 
     
  
   if ((i<1)||((i+S2->len-1)>S1->len))
	   abend(SETL_SYSTEM "the slice index (%d) in FLAT_SET_SLICE are out of range (1,%d)\n",i,(S1->len-S2->len+1));
       

		

   memcpy(S1->str+i-1,S2->str,S2->len);

   unmark_specifier(target);
   target->sp_form = ft_omega;

   
   return;

}


SETL_API void FLAT_FILE_GET(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *A;  
int len;
int s1,s2; 
STRING_ITERATOR(sc)
int i;
char *str,*filename;
FILE *fp;
long retsize;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","flat_file_get");
 
   s1=check_int(SETL_SYSTEM argv,1,ft_short,"integer","flat_file_get");
   s2=check_int(SETL_SYSTEM argv,2,ft_short,"integer","flat_file_get");

 
   ITERATE_STRING_BEGIN(sc,argv[0]);
   
   len = STRING_LEN(argv[0]);

   str=filename=(char*)malloc(len+1);
   if (str==NULL)
   	  giveup(SETL_SYSTEM msg_malloc_error);

 
   ITERATE_STRING_BEGIN(sc,argv[0]);
   for (i=0;i<len;i++) {
      *str++=(unsigned char)ITERATE_STRING_CHAR(sc);
      ITERATE_STRING_NEXT(sc);
   }
   *str=0;
 
  
 
  if ((s1<1)||(s2<1)||(s1>s2))
		giveup(SETL_SYSTEM "the slice parameters in FLAT_FILE_GET are out of range");
		
   fp=fopen(filename,"rb");
   free(filename);
   if (fp==NULL) {
 		unmark_specifier(target);
   		target->sp_form=ft_omega;
   		return;
   }
  
   if (fseek(fp,s1-1,0)==-1) {
   		fclose(fp);
   		unmark_specifier(target);
   		target->sp_form=ft_omega;
   		return;
   }
   len=s2-s1+1;
  
  
   
   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = str_type;
   A->len=len;
   
   A->str = (char *)malloc(A->len+1);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
#ifdef MEMDEBUG
   printf("*");
#endif

   retsize=fread(A->str,1,len,fp);
   if (retsize!=len) {
   		A->len=retsize;
   }
   fclose(fp);
   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
   
   return;

}

SETL_API void FLAT_FILE_PUT(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *A;  
int len;
int s2; 
STRING_ITERATOR(sc)
int i;
char *str,*filename;
FILE *fp;
long retsize;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","flat_file_put");
 
   s2=check_int(SETL_SYSTEM argv,1,ft_short,"integer","flat_file_put");
 
   if ((argv[2].sp_form != ft_opaque)||
       (((argv[2].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",3,"flat_file_put",
         abend_opnd_str(SETL_SYSTEM argv+2));
    
   A = (struct setl_flat *)(argv[2].sp_val.sp_opaque_ptr);    
    
   ITERATE_STRING_BEGIN(sc,argv[0]);
   
   len = STRING_LEN(argv[0]);

   str=filename=(char*)malloc(len+1);
   if (str==NULL)
   	  giveup(SETL_SYSTEM msg_malloc_error);

 
   ITERATE_STRING_BEGIN(sc,argv[0]);
   for (i=0;i<len;i++) {
      *str++=(unsigned char)ITERATE_STRING_CHAR(sc);
      ITERATE_STRING_NEXT(sc);
   }
   *str=0;
 
   
 
  if ((s2<1))
		giveup(SETL_SYSTEM "the slice parameter in FLAT_FILE_PUT is out of range");
		
   fp=fopen(filename,"r+b");
   if (fp==NULL)
   		fp=fopen(filename,"w+b");
   		
   free(filename);
   if (fp==NULL) {
   		unmark_specifier(target);
   		target->sp_form=ft_omega;
   		return;
   }
  
   fseek(fp,s2-1,0);
   fwrite(A->str,1,A->len,fp);
   fclose(fp);
  
   unmark_specifier(target);
   target->sp_form = ft_omega;
   
   
   return;

}

/*
 *  Breakup
 */

SETL_API void BREAKUP_IN(
  SETL_SYSTEM_PROTO
  specifier *string,   
  specifier *target,
  char *breakup)  
{
specifier s;
STRING_ITERATOR(ci)
TUPLE_ITERATOR(ti)
SET_ITERATOR(si)

TUPLE_CONSTRUCTOR(ca)
SET_CONSTRUCTOR(cb)
STRING_CONSTRUCTOR(cs)

int i,j;

   if (string[0].sp_form==ft_tuple) {
      TUPLE_CONSTRUCTOR_BEGIN(ca);

      ITERATE_TUPLE_BEGIN(ti,string[0]) 
      {
      specifier tgt;

             
             BREAKUP_IN(SETL_SYSTEM ti_element,&tgt,breakup);

             s.sp_form = tgt.sp_form;
             s.sp_val.sp_biggest = tgt.sp_val.sp_biggest;
			 TUPLE_ADD_CELL(ca,&s);
            
      }
      ITERATE_TUPLE_END(ti)

      TUPLE_CONSTRUCTOR_END(ca);
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
      return;

   } else if (string[0].sp_form==ft_set) {

      SET_CONSTRUCTOR_BEGIN(cb);

      ITERATE_SET_BEGIN(si,string[0]) 
      {
      specifier tgt;

             BREAKUP_IN(SETL_SYSTEM si_element,&tgt,breakup);
             SET_ADD_CELL(cb,&tgt);
      }
      ITERATE_SET_END(si)

      SET_CONSTRUCTOR_END(cb);
      target->sp_form = ft_set;
      target->sp_val.sp_set_ptr = SET_HEADER(cb);
      return;
	
   }

   check_arg(SETL_SYSTEM string,0,ft_string,"string","breakup");

   ITERATE_STRING_BEGIN(ci,string[0]);

   TUPLE_CONSTRUCTOR_BEGIN(ca);

   i=0;
   j=0;
   if ((STRING_LEN(string[0])>0)&&(breakup[(unsigned char)ITERATE_STRING_CHAR(ci)])) {
     j=1;
   }
   while (i<STRING_LEN(string[0])) {
     /* Get rid of the initial separators */
     while ((i<STRING_LEN(string[0]))&&(breakup[(unsigned char)ITERATE_STRING_CHAR(ci)])) {
	i++;
        j++;
        if (j>1) {
             STRING_CONSTRUCTOR_BEGIN(cs);

             s.sp_form = ft_string;
             s.sp_val.sp_string_ptr =  STRING_HEADER(cs);
             TUPLE_ADD_CELL(ca,&s);
             
        }
        ITERATE_STRING_NEXT(ci);
     }
     j=0;
     if (i<STRING_LEN(string[0])) {
        
       
 
        STRING_CONSTRUCTOR_BEGIN(cs);

        while ((i<STRING_LEN(string[0]))&&(breakup[(unsigned char)ITERATE_STRING_CHAR(ci)]==0)) {
             STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci));
	     i++;
             ITERATE_STRING_NEXT(ci);
             
        }
	    s.sp_form = ft_string;
        s.sp_val.sp_string_ptr = STRING_HEADER(cs);
        TUPLE_ADD_CELL(ca,&s);
       
     }
   }

   TUPLE_CONSTRUCTOR_END(ca);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
   return;

}

SETL_API void BREAKUP(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sc)

char breakup[256];

int i;

   check_arg(SETL_SYSTEM argv,1,ft_string,"string","breakup");

   bzero(&breakup[0],256);

   ITERATE_STRING_BEGIN(sc,argv[1]);

   for (i=0;i<STRING_LEN(argv[1]);i++) {
      breakup[(unsigned char)ITERATE_STRING_CHAR(sc)]=1;
      ITERATE_STRING_NEXT(sc);
   }
 
   unmark_specifier(target);
 
   BREAKUP_IN(SETL_SYSTEM argv,target,breakup);

   return;

}

SETL_API void SINGLE_OUT_IN(
  SETL_SYSTEM_PROTO
  specifier *string,   
  specifier *target,
  char *breakup)  
{
specifier s;
STRING_ITERATOR(ci)
TUPLE_ITERATOR(ti)
SET_ITERATOR(si)

TUPLE_CONSTRUCTOR(ca)
SET_CONSTRUCTOR(cb)
STRING_CONSTRUCTOR(cs)

int i;

   if (string[0].sp_form==ft_tuple) {
      TUPLE_CONSTRUCTOR_BEGIN(ca);

      ITERATE_TUPLE_BEGIN(ti,string[0]) 
      {
      specifier tgt;

             
             SINGLE_OUT_IN(SETL_SYSTEM ti_element,&tgt,breakup);

             s.sp_form = tgt.sp_form;
             s.sp_val.sp_biggest = tgt.sp_val.sp_biggest;
			 TUPLE_ADD_CELL(ca,&s);
            

      }
      ITERATE_TUPLE_END(ti)

      TUPLE_CONSTRUCTOR_END(ca);
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
      return;

   } else if (string[0].sp_form==ft_set) {

      SET_CONSTRUCTOR_BEGIN(cb);

      ITERATE_SET_BEGIN(si,string[0]) 
      {
      specifier tgt;

             SINGLE_OUT_IN(SETL_SYSTEM si_element,&tgt,breakup);
             SET_ADD_CELL(cb,&tgt);
      }
      ITERATE_SET_END(si)

      SET_CONSTRUCTOR_END(cb);
      target->sp_form = ft_set;
      target->sp_val.sp_set_ptr = SET_HEADER(cb);
      return;
	
   }

   check_arg(SETL_SYSTEM string,0,ft_string,"string","single_out");

   ITERATE_STRING_BEGIN(ci,string[0]);

   TUPLE_CONSTRUCTOR_BEGIN(ca);

   i=0;
   while (i<STRING_LEN(string[0])) {
        while ((i<STRING_LEN(string[0]))&&(breakup[(unsigned char)ITERATE_STRING_CHAR(ci)]!=0)) {
           
           
 
           STRING_CONSTRUCTOR_BEGIN(cs);

           STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci));
           i++;
           ITERATE_STRING_NEXT(ci);

		   s.sp_form = ft_string;
           s.sp_val.sp_string_ptr = STRING_HEADER(cs);
           TUPLE_ADD_CELL(ca,&s);
         
             
        }

     
        STRING_CONSTRUCTOR_BEGIN(cs);

        while ((i<STRING_LEN(string[0]))&&(breakup[(unsigned char)ITERATE_STRING_CHAR(ci)]==0)) {
             STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci));
	     i++;
             ITERATE_STRING_NEXT(ci);
             
        }
		s.sp_form = ft_string;
        s.sp_val.sp_string_ptr = STRING_HEADER(cs);
        TUPLE_ADD_CELL(ca,&s);
       


   }

   TUPLE_CONSTRUCTOR_END(ca);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
   return;

}

SETL_API void SINGLE_OUT(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sc)

char breakup[256];

int i;

   check_arg(SETL_SYSTEM argv,1,ft_string,"string","single_out");

   bzero(&breakup[0],256);

   ITERATE_STRING_BEGIN(sc,argv[1]);

   for (i=0;i<STRING_LEN(argv[1]);i++) {
      breakup[(unsigned char)ITERATE_STRING_CHAR(sc)]=1;
      ITERATE_STRING_NEXT(sc);
   }
 
   unmark_specifier(target);
 
   SINGLE_OUT_IN(SETL_SYSTEM argv,target,breakup);

   return;

}

SETL_API void SEGREGATE_IN(
  SETL_SYSTEM_PROTO
  specifier *string,   
  specifier *target,
  char *breakup)  
{

STRING_ITERATOR(ci)
TUPLE_ITERATOR(ti)
SET_ITERATOR(si)

TUPLE_CONSTRUCTOR(ca)
SET_CONSTRUCTOR(cb)
STRING_CONSTRUCTOR(cs)
specifier s;

int i;

   if (string[0].sp_form==ft_tuple) {
      TUPLE_CONSTRUCTOR_BEGIN(ca);

      ITERATE_TUPLE_BEGIN(ti,string[0]) 
      {
      specifier tgt;

             
             SEGREGATE_IN(SETL_SYSTEM ti_element,&tgt,breakup);

             s.sp_form = tgt.sp_form;
             s.sp_val.sp_biggest = tgt.sp_val.sp_biggest;
			 TUPLE_ADD_CELL(ca,&s);
			 

      }
      ITERATE_TUPLE_END(ti)

      TUPLE_CONSTRUCTOR_END(ca);
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
      return;

   } else if (string[0].sp_form==ft_set) {

      SET_CONSTRUCTOR_BEGIN(cb);

      ITERATE_SET_BEGIN(si,string[0]) 
      {
      specifier tgt;

             SEGREGATE_IN(SETL_SYSTEM si_element,&tgt,breakup);
             SET_ADD_CELL(cb,&tgt);
      }
      ITERATE_SET_END(si)

      SET_CONSTRUCTOR_END(cb);
      target->sp_form = ft_set;
      target->sp_val.sp_set_ptr = SET_HEADER(cb);
      return;
	
   }

   check_arg(SETL_SYSTEM string,0,ft_string,"string","segregate");

   ITERATE_STRING_BEGIN(ci,string[0]);

   TUPLE_CONSTRUCTOR_BEGIN(ca);

   i=0;
   while (i<STRING_LEN(string[0])) {
        if (breakup[(unsigned char)ITERATE_STRING_CHAR(ci)]!=0) {
         
           STRING_CONSTRUCTOR_BEGIN(cs);

           while ((i<STRING_LEN(string[0]))&&(breakup[(unsigned char)ITERATE_STRING_CHAR(ci)]!=0)) {
              STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci));
              i++;
              ITERATE_STRING_NEXT(ci);
           }
		   s.sp_form = ft_string;
           s.sp_val.sp_string_ptr = STRING_HEADER(cs);
           TUPLE_ADD_CELL(ca,&s);
             
        }
    
 
        STRING_CONSTRUCTOR_BEGIN(cs);

        while ((i<STRING_LEN(string[0]))&&(breakup[(unsigned char)ITERATE_STRING_CHAR(ci)]==0)) {
             STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci));
	     i++;
             ITERATE_STRING_NEXT(ci);
             
        }
		s.sp_form = ft_string;
        s.sp_val.sp_string_ptr = STRING_HEADER(cs);
        TUPLE_ADD_CELL(ca,&s);
        
 

   }

   TUPLE_CONSTRUCTOR_END(ca);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
   return;

}

SETL_API void SEGREGATE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sc)

char breakup[256];

int i;

   check_arg(SETL_SYSTEM argv,1,ft_string,"string","segregate");

   bzero(&breakup[0],256);

   ITERATE_STRING_BEGIN(sc,argv[1]);

   for (i=0;i<STRING_LEN(argv[1]);i++) {
      breakup[(unsigned char)ITERATE_STRING_CHAR(sc)]=1;
      ITERATE_STRING_NEXT(sc);
   }
 
   unmark_specifier(target);
 
   SEGREGATE_IN(SETL_SYSTEM argv,target,breakup);

   return;

}

SETL_API void KEEP_CHARS(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sc)
STRING_ITERATOR(ci)
STRING_CONSTRUCTOR(cs)
char breakup[256];

int i;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","keep_chars");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","keep_chars");

   bzero(&breakup[0],256);

   ITERATE_STRING_BEGIN(sc,argv[1]);

   for (i=0;i<STRING_LEN(argv[1]);i++) {
      breakup[(unsigned char)ITERATE_STRING_CHAR(sc)]=1;
      ITERATE_STRING_NEXT(sc);
   }
 

   ITERATE_STRING_BEGIN(ci,argv[0]);
   STRING_CONSTRUCTOR_BEGIN(cs);

   i=0;
   while (i<STRING_LEN(argv[0])) {
        if (breakup[(unsigned char)ITERATE_STRING_CHAR(ci)]!=0) {
              STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci));
        }
        i++;
        ITERATE_STRING_NEXT(ci);
   }

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = STRING_HEADER(cs);
   return;

}

SETL_API void SUPPRESS_CHARS(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sc)
STRING_ITERATOR(ci)
STRING_CONSTRUCTOR(cs)
char breakup[256];

int i;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","suppress_chars");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","suppress_chars");

   bzero(&breakup[0],256);

   ITERATE_STRING_BEGIN(sc,argv[1]);

   for (i=0;i<STRING_LEN(argv[1]);i++) {
      breakup[(unsigned char)ITERATE_STRING_CHAR(sc)]=1;
      ITERATE_STRING_NEXT(sc);
   }
 

   ITERATE_STRING_BEGIN(ci,argv[0]);
   STRING_CONSTRUCTOR_BEGIN(cs);

   i=0;
   while (i<STRING_LEN(argv[0])) {
        if (breakup[(unsigned char)ITERATE_STRING_CHAR(ci)]==0) {
              STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci));
        }
        i++;
        ITERATE_STRING_NEXT(ci);
   }

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = STRING_HEADER(cs);
   return;

}


SETL_API void ASCII_VAL(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* string header pointer             */
string_c_ptr_type string_cell;         /* string cell pointer               */

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","ascii_val");


   string_hdr = argv[0].sp_val.sp_string_ptr;

   if (string_hdr->s_length != 1)
       abend(SETL_SYSTEM msg_abs_too_long,abend_opnd_str(SETL_SYSTEM argv));

   string_cell = string_hdr->s_head;


   unmark_specifier(target);
   target->sp_form = ft_short;
   target->sp_val.sp_short_value =
      (unsigned char)(string_cell->s_cell_value[0]);

   return;

}


SETL_API void HEXIFY(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
STRING_ITERATOR(ci)
STRING_CONSTRUCTOR(cs)
char hexbuff[10];
int i;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","hexify");


   ITERATE_STRING_BEGIN(ci,argv[0]);
   STRING_CONSTRUCTOR_BEGIN(cs);

	i=0;
   while (i<STRING_LEN(argv[0])) {
   		sprintf(hexbuff,"%2X",(int)((unsigned char)ITERATE_STRING_CHAR(ci)));
        	if (hexbuff[0]==' ') hexbuff[0]='0';
        	if (hexbuff[1]==' ') hexbuff[1]='0';
        	
              STRING_CONSTRUCTOR_ADD(cs,hexbuff[0]);
              STRING_CONSTRUCTOR_ADD(cs,hexbuff[1]);
        
        ITERATE_STRING_NEXT(ci);
        i++;
   }

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = STRING_HEADER(cs);
   return;

}

SETL_API void CASE_CHANGE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
STRING_ITERATOR(sc)
STRING_ITERATOR(ci)
STRING_CONSTRUCTOR(cs)

int ul_or_lu;

int i;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","case_change");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","case_change");

   ITERATE_STRING_BEGIN(sc,argv[1]);

   if (STRING_LEN(argv[1])!=2)
      abend(SETL_SYSTEM "The string must be ul or lu\n","string",1,"case_change",
            abend_opnd_str(SETL_SYSTEM argv+1));

   if ((unsigned char)ITERATE_STRING_CHAR(sc)=='u') {
      ITERATE_STRING_NEXT(sc);
      if ((unsigned char)ITERATE_STRING_CHAR(sc)=='l')
         ul_or_lu = 0;
      else {
       abend(SETL_SYSTEM "The string must be ul or lu\n","string",1,"case_change",
            abend_opnd_str(SETL_SYSTEM argv+1));
      }
   } else if ((unsigned char)ITERATE_STRING_CHAR(sc)=='l') {
	 ITERATE_STRING_NEXT(sc);
      if ((unsigned char)ITERATE_STRING_CHAR(sc)=='u')
         ul_or_lu = 1;
      else {
       abend(SETL_SYSTEM "The string must be ul or lu\n","string",1,"case_change",
            abend_opnd_str(SETL_SYSTEM argv+1));
      }
   } else {
      abend(SETL_SYSTEM "The string must be ul or lu\n","string",1,"case_change",
            abend_opnd_str(SETL_SYSTEM argv+1));
   }
   ITERATE_STRING_BEGIN(ci,argv[0]);
   STRING_CONSTRUCTOR_BEGIN(cs);

   i=0;
   if (ul_or_lu) {
      while (i<STRING_LEN(argv[0])) {
        if (((unsigned char)ITERATE_STRING_CHAR(ci)>='a')&&((unsigned char)ITERATE_STRING_CHAR(ci)<='z')) {
              STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci)-32);
        } else {
              STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci));
        }
        i++;
        ITERATE_STRING_NEXT(ci);
      }
   } else {
      while (i<STRING_LEN(argv[0])) {
        if (((unsigned char)ITERATE_STRING_CHAR(ci)>='A')&&((unsigned char)ITERATE_STRING_CHAR(ci)<='Z')) {
              STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci)+32);
        } else {
              STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(ci));
        }
        i++;
        ITERATE_STRING_NEXT(ci);
      }
   }

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = STRING_HEADER(cs);

   return;

}


SETL_API void JOIN(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
TUPLE_ITERATOR(ti)
STRING_ITERATOR(sa)
STRING_ITERATOR(sb)
STRING_CONSTRUCTOR(cs)
int tuple_el;

   check_arg(SETL_SYSTEM argv,0,ft_tuple,"tuple","join");
   check_arg(SETL_SYSTEM argv,1,ft_string,"string","join");

   STRING_CONSTRUCTOR_BEGIN(cs);

   tuple_el = 0;
   ITERATE_TUPLE_BEGIN(ti,argv[0]) 
   {
   int i;

       if (ti_element->sp_form!=ft_string) {
          abend(SETL_SYSTEM "Tuple in JOIN must have string elements");
       }
       ITERATE_STRING_BEGIN(sb,(*ti_element));
       for (i=0;i<STRING_LEN((*ti_element));i++) {
         STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(sb));
         ITERATE_STRING_NEXT(sb);
       }
       tuple_el ++;
       
       if (tuple_el!=TUPLE_LEN(argv[0])) {
          ITERATE_STRING_BEGIN(sa,argv[1]);
          for (i=0;i<STRING_LEN(argv[1]);i++) {
            STRING_CONSTRUCTOR_ADD(cs,(unsigned char)ITERATE_STRING_CHAR(sa));
            ITERATE_STRING_NEXT(sa);
          }
       }

   }
   ITERATE_TUPLE_END(ti)

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = STRING_HEADER(cs);

}


SETL_API void FLAT_TOTO_PREPARE(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S;  
struct setl_flat *A;  
int len; 
int i;
unsigned char *sp,*dp;
int32 *tp;
char cset[256];

   for (i=0;i<256;i++) cset[i]=0;
   cset['a']=1;
   cset['c']=1;
   cset['t']=1;
   cset['g']=1;

   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat toto prepare",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   S = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   len=S->len;
   
   A = (struct setl_flat *)(malloc(sizeof(struct setl_flat)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = 65536+str_type;
   A->len=len;
   
   A->str = (char *)malloc(A->len*2+256*sizeof(int32)+1024);
   if (A->str==NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   sp=S->str;
   dp=A->str+256*sizeof(int32)+S->len+256; /* Point to match counters */
   for (i=0;i<S->len;i++) {
	*dp++=0; /* Erase match counters */
   }

   dp=A->str+256*sizeof(int32); /* point to match table */

   tp=(int32*)A->str;

   /* Clear Alphabet Table */
   for (i=0;i<256;i++) tp[i]=-1;
  
   for (i=0;i<S->len;i++) {
   unsigned char c;
	c=*sp++;
	if (cset[c]!=0) {
	if (tp[c]<0) {
		*dp=0; /* First occurence of c */
        } else {
		if ((i-tp[c])>255)  { /* Error (for now) at most 255 chars
					between 2 occurences of the same
					letter */
			free(A->str);
			free(A);
			printf("Error in position %d char %c previous %d \n",
				i,c,tp[c]);
   			unmark_specifier(target);
			target->sp_form = ft_omega;
			return;
		}		
		*dp=(i-tp[c]);
	}
	tp[c]=i;
	}
	dp++;
   }

   dp=A->str+256*sizeof(int32)+S->len+256+S->len; /* point to string cache */

   for (i=0;i<256;i++) {
	*dp++=0; /* Erase string cache */
   }
   *dp=0; /* Set cache as clear */

   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;
   
   return;

}



SETL_API void FLAT_TOTO_PRINT(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S;  
int len; 
int i;
unsigned char *sp,*dp;
int32 *tp;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat toto prepare",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   S = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   tp=(int32 *)S->str;

   printf("The table is : \n");
   for (i=0;i<256;i++) {
	if (tp[i]>0) {
	printf("%c : %d   ",(unsigned char)i,tp[i]);
	}
   }
   printf("\nThe match table is  is : \n");

   sp=S->str+256*sizeof(int32); /* point to match table */
   for (i=0;i<S->len;i++) {
	printf("%d ",(int)*sp++);

   }
   printf("\n");
   dp=S->str+256*sizeof(int32)+S->len+256+S->len; /* point to string cache */
   if (*(dp+256)==0) printf("The cache is empty\n"); else 
   {
    printf("Cache contains:\n");
    for (i=0;i<*(dp+256);i++) printf("%c",*(dp+i));
    printf("\n");

   } 
   
}

SETL_API void FLAT_TOTO_MATCH(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S1;   
struct setl_flat *S2; 
TUPLE_CONSTRUCTOR(ca)
TUPLE_CONSTRUCTOR(cb)
unsigned char *s1,*s2,*s3;
unsigned char *diffp;
unsigned char *match_counters;
unsigned char *cachep;
specifier s;
int number;
int32 *tp;
int i;

	/*
	printf("Staring match...\n");
	*/

   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat add",
         abend_opnd_str(SETL_SYSTEM argv+0));

 
   if ((argv[1].sp_form != ft_opaque)||
       (((argv[1].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",2,"flat add",
         abend_opnd_str(SETL_SYSTEM argv+1));

   number=check_int(SETL_SYSTEM argv,2,ft_short,"integer","FLAT_TOTO_MATCH");

   S1 = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   S2 = (struct setl_flat *)(argv[1].sp_val.sp_opaque_ptr);
  
   if ((S2->len)>250)
   		giveup(SETL_SYSTEM "Length of search string in FLAT_TOTO_MATCH >250");
   
   tp=(int32*)S1->str;
   s2=S2->str;

   /* point to string cache */
   cachep=S1->str+256*sizeof(int32)+S1->len+256+S1->len; 
   diffp=S1->str+256*sizeof(int32); /* point to match table */

   if (*(cachep+256)==0) { /* The cache is empty */
      memcpy(cachep,S2->str,S2->len);
      *(cachep+256)=S2->len;

      /* Point to match counters */
      match_counters=S1->str+256*sizeof(int32)+S1->len+256; 
      diffp=S1->str+256*sizeof(int32); /* point to match table */

      for (i=0;i<S2->len;i++) { 
      int j;
      unsigned char c;
      unsigned char *mc;
      unsigned char *di;

  	 c=*s2++;
	 if (tp[c]>0) {
                mc=match_counters;
      		di=diffp;
		di=di+tp[c]; mc=mc+tp[c]-i;
		(*mc)++;
	        while ((j=*di)>0) {
		  mc-=j; di-=j;
		  (*mc)++;
		} 
	 }
	

	
   }

   } else {
      /* For now I am assuming equal length */
      /* cachep points to the first char in the cache */

      /* Point to match counters */
      match_counters=S1->str+256*sizeof(int32)+S1->len+256; 
      diffp=S1->str+256*sizeof(int32); /* point to match table */

      for (i=0;i<S2->len;i++) { 
      int j;
      unsigned char c,d;
      unsigned char *mc;
      unsigned char *di;

  	 c=*s2++;
         d=*cachep;
         if (c!=d) {
	    if (tp[d]>0) {
            	   mc=match_counters;
      		   di=diffp;
	   	   di=di+tp[d]; mc=mc+tp[d]-i;
		   (*mc)--;
	           while ((j=*di)>0) {
		     mc-=j; di-=j;
		     (*mc)--;
		   } 
	    }
	    if (tp[c]>0) {
            	   mc=match_counters;
      		   di=diffp;
	   	   di=di+tp[c]; mc=mc+tp[c]-i;
		   (*mc)++;
	           while ((j=*di)>0) {
		     mc-=j; di-=j;
		     (*mc)++;
		   } 
	    }
         }
         cachep++;
	
      }
   }

	/*
	printf("Searching Scores...\n");
	*/
   if (number==0) {

   TUPLE_CONSTRUCTOR_BEGIN(ca);

   match_counters=S1->str+256*sizeof(int32)+S1->len+256; 
   for (i=0;i<S1->len;i++) {

	s.sp_form = ft_short;
	s.sp_val.sp_short_value = *match_counters++;
	TUPLE_ADD_CELL(ca,&s);

   }
 
   TUPLE_CONSTRUCTOR_END(ca);
 
   } else { /* We want the best number scores */
        int bestscore=-1;
        int worstscore=32000;
	int found=0;
	int j,k;
	int *tempscores=(int*)malloc(sizeof(int)*(number+2)*2);
	for (i=0;i<number*2;i++) tempscores[i]=0;
	

        match_counters=S1->str+256*sizeof(int32)+S1->len+256; 
        for (i=0;i<S1->len;i++) {
	   k=*match_counters++;
	   if (k>0) {
	   if (k>=bestscore) {
		memcpy(&tempscores[2],&tempscores[0],sizeof(int)*2*number);
		tempscores[0]=k;
		tempscores[1]=i;
		bestscore=k;
		if (found<number) found++;
	   } else { /* We didn't find the best, but we need more */
	        int inserted=0;
		for (j=1;j<found;j++) {
		  if (k>tempscores[j*2]) {
		     memcpy(&tempscores[j*2+2],&tempscores[j*2],sizeof(int)*2*(found-j+1));
		     tempscores[j*2]=k;
		     tempscores[j*2+1]=i;
		     inserted=1;
		     j=found;
		  }
		}
		if (inserted==0) { /* Insert at the end */
		   if (found<number) {
		     found++;
		     tempscores[found*2-2]=k;
		     tempscores[found*2-1]=i;
                   }
		} 

	   }
	   }
	}
	/*
	printf("Done...\n");
	*/

   	TUPLE_CONSTRUCTOR_BEGIN(ca);

   	for (i=0;i<found;i++) {


   	TUPLE_CONSTRUCTOR_BEGIN(cb);
	
	s.sp_form = ft_short;
	s.sp_val.sp_short_value = tempscores[i*2];
	TUPLE_ADD_CELL(cb,&s);
	
	
	s.sp_form = ft_short;
	s.sp_val.sp_short_value = tempscores[i*2+1]+1;
	TUPLE_ADD_CELL(cb,&s);

   	TUPLE_CONSTRUCTOR_END(cb);

	s.sp_form = ft_tuple;
	s.sp_val.sp_tuple_ptr = TUPLE_HEADER(cb);
	TUPLE_ADD_CELL(ca,&s);
	
	}

   	TUPLE_CONSTRUCTOR_END(ca);
	free(tempscores);

   }



   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
   return;

}

SETL_API void FLAT_TOTO_CLEAR(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S;  
int len; 
int i;
unsigned char *sp,*dp;
int32 *tp;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat toto clear",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   S = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);

   dp=S->str+256*sizeof(int32)+S->len+256; /* Point to match counters */
   for (i=0;i<S->len;i++) {
	*dp++=0; /* Erase match counters */
   }

   dp=S->str+256*sizeof(int32)+S->len+256+S->len; /* point to string cache */
   *(dp+256)=0; /* Clear Cache */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;
   
}

void SerialDateToDMY(int nSerialDate, int *nDay, int *nMonth, int *nYear)
{
    int l,n,i,j;

    l = nSerialDate + 68569 + 2415019;
    n = (int)(( 4 * l ) / 146097);
            l = l - (int)(( 146097 * n + 3 ) / 4);
    i = (int)(( 4000 * ( l + 1 ) ) / 1461001);
        l = l - (int)(( 1461 * i ) / 4) + 31;
    j = (int)(( 80 * l ) / 2447);
     *nDay = l - (int)(( 2447 * j ) / 80);
        l = (int)(j / 11);
        *nMonth = j + 2 - ( 12 * l );
    *nYear = 100 * ( n - 49 ) + i + l;
}

SETL_API void FLAT_SLICES_TO_SETL(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S1;   
struct setl_flat *S2; 
STRING_CONSTRUCTOR(cs)
TUPLE_CONSTRUCTOR(ca)
unsigned char *flat,*template;
specifier sd,s,ss,si,s256;
int number;
int i,j,len,conv;
int32 result,hi_bits;
long lresult;
char sign;
int d,m,y;
unsigned char b;
int islong;
char datebuffer[128];
i_real_ptr_type real_ptr;     
double fresult;
char tmpstring[100];
  


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat_slices_to_setl",
         abend_opnd_str(SETL_SYSTEM argv+0));

   number=check_int(SETL_SYSTEM argv,1,ft_short,"integer","FLAT_SLICES_TO_SETL");
 
   if ((argv[2].sp_form != ft_opaque)||
       (((argv[2].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",3,"flat_slices_to_setl",
         abend_opnd_str(SETL_SYSTEM argv+2));


   S1 = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   S2 = (struct setl_flat *)(argv[2].sp_val.sp_opaque_ptr);
  
   flat=S1->str+number;
   template=S2->str;
   si.sp_form=ft_short;
   sd.sp_form=ft_real;
   s256.sp_form=ft_short;
   s256.sp_val.sp_short_value=256;

   TUPLE_CONSTRUCTOR_BEGIN(ca);

   for (j=0;j<S2->len/2;j++) { 
	/* Loop into template string */
        len=*template++;
        conv=*template++;
	switch (conv) {
	  case 10:
   		STRING_CONSTRUCTOR_BEGIN(cs);
		i=0;
		while (i<len) {
        	  STRING_CONSTRUCTOR_ADD(cs,*(flat+i));
       		   i++;
  		}
	        ss.sp_form=ft_string;
       		ss.sp_val.sp_string_ptr = STRING_HEADER(cs);
                TUPLE_ADD_CELL(ca,&ss);
		break;
	  case 9: /* DATE */
		result=0;
		i=0;
		while (i<len) {
		   b=(unsigned char)*(flat+i);
		     
		     result=result*256+b;
		     i++;
		}
		result+=1;
		SerialDateToDMY(result,&d,&m,&y);
	 	sprintf(&datebuffer,"%04d-%02d-%02d",y,m,d);
   		STRING_CONSTRUCTOR_BEGIN(cs);
		i=0;
		while (i<10) {
        	  STRING_CONSTRUCTOR_ADD(cs,datebuffer[i]);
       		   i++;
  		}
	        ss.sp_form=ft_string;
       		ss.sp_val.sp_string_ptr = STRING_HEADER(cs);
                TUPLE_ADD_CELL(ca,&ss);
		break;
	  case 0:
		break;
	  case 3:
	  case 4:
	  case 2:
	  case 1:
		islong=0;
		result=0;
		i=0;
		sign=1;
		if (conv>2) { /* get sign bit */
		   if (((unsigned char)*(flat)>>7)==0) sign=-1;
		}
		while (i<len) {
		   b=(unsigned char)*(flat+i);
		   if ((i==0)&&(conv>2)) b=b&127;
		     
		   if (islong==0) {
		     result=result*256+sign*b;
  		     if (!(!(hi_bits = result & INT_HIGH_BITS) ||
                        hi_bits == INT_HIGH_BITS)) {
			islong=1;
			s.sp_form=ft_omega;
			short_to_long(SETL_SYSTEM &s,result);
		       } 
		     } else {
			integer_multiply(SETL_SYSTEM &s,&s,&s256);
			si.sp_val.sp_short_value=sign*b;
			integer_add(SETL_SYSTEM &s,&s,&si);
		     }		
		     i++;
		}
                if (islong==0) {
	  	   si.sp_val.sp_short_value = result;
		   if ((conv==2)||(conv==4)) {
		      TUPLE_ADD_CELL(ca,&si);
		   } else {
			/* convert to string */
   		      ss.sp_form=ft_omega;
		      setl2_str(SETL_SYSTEM 1,&si,&ss);
		      TUPLE_ADD_CELL(ca,&ss);
		   }
		} else {
		   if ((conv==2)||(conv==4)) {
		      TUPLE_ADD_CELL(ca,&s);
		   } else {
			/* convert to string */
   		      ss.sp_form=ft_omega;
		      setl2_str(SETL_SYSTEM 1,&s,&ss);
		      TUPLE_ADD_CELL(ca,&ss);
		   }
	
 
                }
	       break;
	  case 5:
	  case 6:
	  case 7:
	  case 8:
		islong=0;
		result=0;
		i=0;
		sign=1;
		if (conv>6) { /* get sign bit */
		   if (((unsigned char)*(flat)>>7)==0) sign=-1;
		}
		while (i<len) {
		   b=(unsigned char)*(flat+i);
		   if ((i==0)&&(conv>2)) b=b&127;
		     
		   if (islong==0) {
		     result=result*256+sign*b;
  		     if (!(!(hi_bits = result & INT_HIGH_BITS) ||
                        hi_bits == INT_HIGH_BITS)) {
			islong=1;
			s.sp_form=ft_omega;
			short_to_long(SETL_SYSTEM &s,result);
		       } 
		     } else {
			integer_multiply(SETL_SYSTEM &s,&s,&s256);
			si.sp_val.sp_short_value=sign*b;
			integer_add(SETL_SYSTEM &s,&s,&si);
		     }		
		     i++;
		}
                if (islong==1) {
		   fresult=(long_to_double(SETL_SYSTEM &s))/100;
		} else {
		   fresult=(double)(result)/100;
		}
	           i_get_real(real_ptr);
            	   sd.sp_form = ft_real;
            	   sd.sp_val.sp_real_ptr = real_ptr;
            	   real_ptr->r_use_count = 1;
                   real_ptr->r_value = fresult;

		   if ((conv==6)||(conv==8)) {
		      TUPLE_ADD_CELL(ca,&sd);
		   } else {
			/* convert to string */
 	 	      sprintf(tmpstring,"%#.2f",fresult);
			/*printf("the string is %s\n",tmpstring);*/
		      unmark_specifier(&sd);
   		STRING_CONSTRUCTOR_BEGIN(cs);
		i=0;
		while (i<strlen(tmpstring)) {
        	  STRING_CONSTRUCTOR_ADD(cs,tmpstring[i]);
       		   i++;
  		}
	        ss.sp_form=ft_string;
       		ss.sp_val.sp_string_ptr = STRING_HEADER(cs);
                TUPLE_ADD_CELL(ca,&ss);
		}
  
	       break;
	  default:
	       break;
	}
	flat+=len;


   }
 
   TUPLE_CONSTRUCTOR_END(ca);
 
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
   return;

}


SETL_API void FLAT_SLICES_TO_SETL_TUP(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_flat *S1;   
struct setl_flat *S2; 
STRING_CONSTRUCTOR(cs)
TUPLE_CONSTRUCTOR(ca)
TUPLE_CONSTRUCTOR(cb)
unsigned char *flat,*template;
specifier sd,s,ss,si,s256;
int number;
int i,j,len,conv;
int32 result,hi_bits;
long lresult;
char sign;
int d,m,y;
int number_of_tuples,kk;
unsigned char b;
int islong;
char datebuffer[128];
i_real_ptr_type real_ptr;     
double fresult;
char tmpstring[100];
  


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",1,"flat_slices_to_setl_tup",
         abend_opnd_str(SETL_SYSTEM argv+0));

   number=check_int(SETL_SYSTEM argv,1,ft_short,"integer","FLAT_SLICES_TO_SETL_TUP");
 
   if ((argv[2].sp_form != ft_opaque)||
       (((argv[2].sp_val.sp_opaque_ptr->type)&65535)!=str_type))
       abend(SETL_SYSTEM msg_bad_arg,"flat string",3,"flat_slices_to_setl_tup",
         abend_opnd_str(SETL_SYSTEM argv+2));

    number_of_tuples=check_int(SETL_SYSTEM argv,3,ft_short,"integer","FLAT_SLICES_TO_SETL_TUP");
  
   S1 = (struct setl_flat *)(argv[0].sp_val.sp_opaque_ptr);
   S2 = (struct setl_flat *)(argv[2].sp_val.sp_opaque_ptr);
  
   flat=S1->str+number;

   si.sp_form=ft_short;
   sd.sp_form=ft_real;
   s256.sp_form=ft_short;
   s256.sp_val.sp_short_value=256;
   
   TUPLE_CONSTRUCTOR_BEGIN(cb);
   
   for (kk=1;kk<=number_of_tuples;kk++) {
   		
      template=S2->str;

   TUPLE_CONSTRUCTOR_BEGIN(ca);

   for (j=0;j<S2->len/2;j++) { 
	/* Loop into template string */
        len=*template++;
        conv=*template++;
	switch (conv) {
	  case 10:
   		STRING_CONSTRUCTOR_BEGIN(cs);
		i=0;
		while (i<len) {
        	  STRING_CONSTRUCTOR_ADD(cs,*(flat+i));
       		   i++;
  		}
	        ss.sp_form=ft_string;
       		ss.sp_val.sp_string_ptr = STRING_HEADER(cs);
                TUPLE_ADD_CELL(ca,&ss);
		break;
	  case 9: /* DATE */
		result=0;
		i=0;
		while (i<len) {
		   b=(unsigned char)*(flat+i);
		     
		     result=result*256+sign*b;
		     i++;
		}
		result+=1;
		SerialDateToDMY(result,&d,&m,&y);
	 	sprintf(&datebuffer,"%04d-%02d-%02d",y,m,d);
   		STRING_CONSTRUCTOR_BEGIN(cs);
		i=0;
		while (i<10) {
        	  STRING_CONSTRUCTOR_ADD(cs,datebuffer[i]);
       		   i++;
  		}
	        ss.sp_form=ft_string;
       		ss.sp_val.sp_string_ptr = STRING_HEADER(cs);
                TUPLE_ADD_CELL(ca,&ss);
		break;
	  case 0:
		break;
	  case 3:
	  case 4:
	  case 2:
	  case 1:
		islong=0;
		result=0;
		i=0;
		sign=1;
		if (conv>2) { /* get sign bit */
		   if (((unsigned char)*(flat)>>7)==0) sign=-1;
		}
		while (i<len) {
		   b=(unsigned char)*(flat+i);
		   if ((i==0)&&(conv>2)) b=b&127;
		     
		   if (islong==0) {
		     result=result*256+sign*b;
  		     if (!(!(hi_bits = result & INT_HIGH_BITS) ||
                        hi_bits == INT_HIGH_BITS)) {
			islong=1;
			s.sp_form=ft_omega;
			short_to_long(SETL_SYSTEM &s,result);
		       } 
		     } else {
			integer_multiply(SETL_SYSTEM &s,&s,&s256);
			si.sp_val.sp_short_value=sign*b;
			integer_add(SETL_SYSTEM &s,&s,&si);
		     }		
		     i++;
		}
                if (islong==0) {
	  	   si.sp_val.sp_short_value = result;
		   if ((conv==2)||(conv==4)) {
		      TUPLE_ADD_CELL(ca,&si);
		   } else {
			/* convert to string */
   		      ss.sp_form=ft_omega;
		      setl2_str(SETL_SYSTEM 1,&si,&ss);
		      TUPLE_ADD_CELL(ca,&ss);
		   }
		} else {
		   if ((conv==2)||(conv==4)) {
		      TUPLE_ADD_CELL(ca,&s);
		   } else {
			/* convert to string */
   		      ss.sp_form=ft_omega;
		      setl2_str(SETL_SYSTEM 1,&s,&ss);
		      TUPLE_ADD_CELL(ca,&ss);
		   }
	
 
                }
	       break;
	  case 5:
	  case 6:
	  case 7:
	  case 8:
		islong=0;
		result=0;
		i=0;
		sign=1;
		if (conv>6) { /* get sign bit */
		   if (((unsigned char)*(flat)>>7)==0) sign=-1;
		}
		while (i<len) {
		   b=(unsigned char)*(flat+i);
		   if ((i==0)&&(conv>2)) b=b&127;
		     
		   if (islong==0) {
		     result=result*256+sign*b;
  		     if (!(!(hi_bits = result & INT_HIGH_BITS) ||
                        hi_bits == INT_HIGH_BITS)) {
			islong=1;
			s.sp_form=ft_omega;
			short_to_long(SETL_SYSTEM &s,result);
		       } 
		     } else {
			integer_multiply(SETL_SYSTEM &s,&s,&s256);
			si.sp_val.sp_short_value=sign*b;
			integer_add(SETL_SYSTEM &s,&s,&si);
		     }		
		     i++;
		}
                if (islong==1) {
		   fresult=(long_to_double(SETL_SYSTEM &s))/100;
		} else {
		   fresult=(double)(result)/100;
		}
	           i_get_real(real_ptr);
            	   sd.sp_form = ft_real;
            	   sd.sp_val.sp_real_ptr = real_ptr;
            	   real_ptr->r_use_count = 1;
                   real_ptr->r_value = fresult;

		   if ((conv==6)||(conv==8)) {
		      TUPLE_ADD_CELL(ca,&sd);
		   } else {
			/* convert to string */
 	 	      sprintf(tmpstring,"%#.2f",fresult);
			/*printf("the string is %s\n",tmpstring);*/
		      unmark_specifier(&sd);
   		STRING_CONSTRUCTOR_BEGIN(cs);
		i=0;
		while (i<strlen(tmpstring)) {
        	  STRING_CONSTRUCTOR_ADD(cs,tmpstring[i]);
       		   i++;
  		}
	        ss.sp_form=ft_string;
       		ss.sp_val.sp_string_ptr = STRING_HEADER(cs);
                TUPLE_ADD_CELL(ca,&ss);
		}
  
	       break;
	  default:
	       break;
	}
	flat+=len;


   }
 
   TUPLE_CONSTRUCTOR_END(ca);
   ss.sp_form = ft_tuple;
   ss.sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
   TUPLE_ADD_CELL(cb,&ss);
   
   }
   TUPLE_CONSTRUCTOR_END(cb);
   
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(cb);
   return;

}
