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
 *  This is a compression Native Package for SETL2
 *
 */

/* SETL2 system header files */


#include "macros.h"
#include "zlib.h"
#include "unzip.h"
#ifdef unix
#include "utime.h"

#endif

#define YES 1
#define NO  0


struct setl_zip {                     /* Native Object Structure           */
   int32 use_count;                    /* Reference Count                   */
   int32 type;                         /* Encodes Type and Subtype          */                         
   char *ptr;
};


#define FLAT_TYPE         1            /* Flat String Setl Object           */
 
static int32 zip_type;                 /* Store the type assigned to us by  */
                                       /* the SETL runtime                  */

 static void internal_destructor(struct setl_zip *spec)
{
int subtyp;

    if ((spec!=NULL)&&((spec->type&65535)==zip_type)) {
    
           if (spec->ptr!=NULL) {
           		//free(spec->str);
           		spec->ptr=NULL;
           		}
        }
    

}


SETL_API int32 ZIP_PAK__INIT(
   SETL_SYSTEM_PROTO_VOID
   )
{
   zip_type=register_type(SETL_SYSTEM "zip utilities",internal_destructor);
   if (zip_type==0) return 1;
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


SETL_API void UNCOMPRESS(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

int len,len2,err;
STRING_CONSTRUCTOR(cs)
STRING_ITERATOR(sc)
int i;
char *str,*uncompr;
char *compr;
long retsize;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","uncompress");
 
 
   ITERATE_STRING_BEGIN(sc,argv[0]);
   len = STRING_LEN(argv[0]);
  
   compr=str=(char*)malloc(len+1);
   if (str==NULL)
   	  giveup(SETL_SYSTEM msg_malloc_error);
 
   ITERATE_STRING_BEGIN(sc,argv[0]);
   for (i=0;i<len;i++) {
      *str++=ITERATE_STRING_CHAR(sc);
      ITERATE_STRING_NEXT(sc);
   }
   *str=0;
   
   len2 = *(int32*)(compr)+128;
  
 
   uncompr=str=(char*)malloc(len2+1);
   if (str==NULL)
   	  giveup(SETL_SYSTEM msg_malloc_error);

 
  
   err = uncompress(uncompr, &len2, (const Bytef*)compr+sizeof(int32), len);
   if (err!=0) 
   		giveup(SETL_SYSTEM "error uncompressing string");
   		
   
   STRING_CONSTRUCTOR_BEGIN(cs);

   i=0;
   str=uncompr;
   while (i<len2) {
      
        STRING_CONSTRUCTOR_ADD(cs,*str++);
        i++;
   }
   free(uncompr);
   free(compr);

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = STRING_HEADER(cs);
   return;

  }

SETL_API void COMPRESS(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

int len,len2,err;
STRING_CONSTRUCTOR(cs)
STRING_ITERATOR(sc)
int i;
char *str,*uncompr;
char *compr;
long retsize;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","compress");
 
 
   ITERATE_STRING_BEGIN(sc,argv[0]);
   
   len2 = len = STRING_LEN(argv[0]);
   len2 = len2 * 1.02 + 12;

   compr=str=(char*)malloc(len2+1+sizeof(int32));
   if (str==NULL)
   	  giveup(SETL_SYSTEM msg_malloc_error);

   uncompr=str=(char*)malloc(len+1);
   if (str==NULL)
   	  giveup(SETL_SYSTEM msg_malloc_error);

 
   ITERATE_STRING_BEGIN(sc,argv[0]);
   for (i=0;i<len;i++) {
      *str++=ITERATE_STRING_CHAR(sc);
      ITERATE_STRING_NEXT(sc);
   }
   *str=0;
 
   str=compr+sizeof(int32);
   err = compress(str, &len2, (const Bytef*)uncompr, len);
   if (err!=0) 
   		giveup(SETL_SYSTEM "error compressing string");
   		
   *(int32*)(compr)=len;
   STRING_CONSTRUCTOR_BEGIN(cs);

   
   i=0;
   str=compr;
   while (i<(len2+sizeof(int32))) {
      
        STRING_CONSTRUCTOR_ADD(cs,*str++);
        i++;
   }
   free(uncompr);
   free(compr);

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = STRING_HEADER(cs);
   return;

  }

SETL_API void OPEN_ZIP(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
struct setl_zip *A; /* w */ 
int len;
unzFile uf=NULL;
STRING_ITERATOR(sc)
int i;
char *str,*filename;

   check_arg(SETL_SYSTEM argv,0,ft_string,"string","open_zip");
 
 
   ITERATE_STRING_BEGIN(sc,argv[0]);
   len = STRING_LEN(argv[0]);
   
   
   str=filename=(char*)malloc(len+1);
   if (str==NULL)
   	  giveup(SETL_SYSTEM msg_malloc_error);

 
   ITERATE_STRING_BEGIN(sc,argv[0]);
   for (i=0;i<len;i++) {
      *str++=ITERATE_STRING_CHAR(sc);
      ITERATE_STRING_NEXT(sc);
   }
   *str=0;
   
   uf = unzOpen(filename);
   free(filename);
   if (uf==NULL) {
		unmark_specifier(target);
		target->sp_form = ft_omega;
		return;
   }
   
   A = (struct setl_zip *)(malloc(sizeof(struct setl_zip)));
   if (A == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   A->use_count = 1;
   A->type = zip_type;
   A->ptr=uf;
   

   unmark_specifier(target);
   target->sp_form = ft_opaque;
   target->sp_val.sp_opaque_ptr = (opaque_item_ptr_type)A;

}

SETL_API void CLOSE_ZIP(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_zip *A; /* w */ 
int len;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=zip_type))
       abend(SETL_SYSTEM msg_bad_arg,"zip object",1,"close_zip",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   A = (struct setl_zip *)(argv[0].sp_val.sp_opaque_ptr);
	
   unzCloseCurrentFile(A->ptr);

   unmark_specifier(target);
   target->sp_form = ft_omega;
   return;

}

SETL_API void DEBUG_ZIP(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_zip *A; /* w */ 
int len;
unzFile uf;

uLong i;
unz_global_info gi;
int err;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=zip_type))
       abend(SETL_SYSTEM msg_bad_arg,"zip object",1,"debug_zip",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   A = (struct setl_zip *)(argv[0].sp_val.sp_opaque_ptr);
   uf = A->ptr;	
  unzGoToFirstFile((unzFile)uf);	

	err = unzGetGlobalInfo (uf,&gi);
	if (err!=UNZ_OK)
		printf("error %d with zipfile in unzGetGlobalInfo \n",err);
    printf(" Length  Method   Size  Ratio   Date    Time   CRC-32     Name\n");
    printf(" ------  ------   ----  -----   ----    ----   ------     ----\n");
	for (i=0;i<gi.number_entry;i++)
	{
		char filename_inzip[256];
		unz_file_info file_info;
		uLong ratio=0;
		const char *string_method;
		err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
		if (err!=UNZ_OK)
		{
			printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
			break;
		}
		if (file_info.uncompressed_size>0)
			ratio = (file_info.compressed_size*100)/file_info.uncompressed_size;

		if (file_info.compression_method==0)
			string_method="Stored";
		else
		if (file_info.compression_method==Z_DEFLATED)
		{
			uInt iLevel=(uInt)((file_info.flag & 0x6)/2);
			if (iLevel==0)
			  string_method="Defl:N";
			else if (iLevel==1)
			  string_method="Defl:X";
			else if ((iLevel==2) || (iLevel==3))
			  string_method="Defl:F"; /* 2:fast , 3 : extra fast*/
		}
		else
			string_method="Unkn. ";

		printf("%7lu  %6s %7lu %3lu%%  %2.2lu-%2.2lu-%2.2lu  %2.2lu:%2.2lu  %8.8lx   %s\n",
			    file_info.uncompressed_size,string_method,file_info.compressed_size,
				ratio,
				(uLong)file_info.tmu_date.tm_mon + 1,
                (uLong)file_info.tmu_date.tm_mday,
				(uLong)file_info.tmu_date.tm_year % 100,
				(uLong)file_info.tmu_date.tm_hour,(uLong)file_info.tmu_date.tm_min,
				(uLong)file_info.crc,filename_inzip);
		if ((i+1)<gi.number_entry)
		{
			err = unzGoToNextFile(uf);
			if (err!=UNZ_OK)
			{
				printf("error %d with zipfile in unzGoToNextFile\n",err);
				break;
			}
		}
	}


   unmark_specifier(target);
   target->sp_form = ft_omega;
   return;

}

SETL_API void LIST_ZIP(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{

struct setl_zip *A; /* w */ 
int len;
unzFile uf;
specifier s;
TUPLE_CONSTRUCTOR(ca)
TUPLE_CONSTRUCTOR(cb)
STRING_CONSTRUCTOR(sa)
uLong i;
int z,j;
unz_global_info gi;
int err;


   if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=zip_type))
       abend(SETL_SYSTEM msg_bad_arg,"zip object",1,"list_zip",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   A = (struct setl_zip *)(argv[0].sp_val.sp_opaque_ptr);
   uf = A->ptr;	
  
    unzGoToFirstFile((unzFile)uf);	

	err = unzGetGlobalInfo (uf,&gi);
	
    TUPLE_CONSTRUCTOR_BEGIN(ca);

        
	       
	if (err!=UNZ_OK) 
		giveup(SETL_SYSTEM "error with zipfile in unzGetGlobalInfo");
		  
	for (i=0;i<gi.number_entry;i++)
	{
		char filename_inzip[256];
		unz_file_info file_info;
		uLong ratio=0;
		const char *string_method;
		err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
		if (err!=UNZ_OK)
		{
			giveup(SETL_SYSTEM "error with zipfile in unzGetCurrentFileInfo");
			break;
		}
		if (file_info.uncompressed_size>0)
			ratio = (file_info.compressed_size*100)/file_info.uncompressed_size;

		if (file_info.compression_method==0)
			string_method="Stored";
		else
		if (file_info.compression_method==Z_DEFLATED)
		{
			uInt iLevel=(uInt)((file_info.flag & 0x6)/2);
			if (iLevel==0)
			  string_method="Defl:N";
			else if (iLevel==1)
			  string_method="Defl:X";
			else if ((iLevel==2) || (iLevel==3))
			  string_method="Defl:F"; /* 2:fast , 3 : extra fast*/
		}
		else
			string_method="Unkn. ";


	    
   
    	TUPLE_CONSTRUCTOR_BEGIN(cb);
	  	

		STRING_CONSTRUCTOR_BEGIN(sa);
		j=strlen(filename_inzip);
		z=0;
		while (z<j) {
			 STRING_CONSTRUCTOR_ADD(sa,filename_inzip[z]);
			 z++;
		}

		s.sp_form=ft_string;
		s.sp_val.sp_string_ptr = STRING_HEADER(sa);
		TUPLE_ADD_CELL(cb,&s);


		s.sp_form = ft_short;
		s.sp_val.sp_short_value=file_info.uncompressed_size;
		TUPLE_ADD_CELL(cb,&s);

		TUPLE_CONSTRUCTOR_END(cb);
		   
		/*    	
		printf("%7lu  %6s %7lu %3lu%%  %2.2lu-%2.2lu-%2.2lu  %2.2lu:%2.2lu  %8.8lx   %s\n",
			    file_info.uncompressed_size,string_method,file_info.compressed_size,
				ratio,
				(uLong)file_info.tmu_date.tm_mon + 1,
                (uLong)file_info.tmu_date.tm_mday,
				(uLong)file_info.tmu_date.tm_year % 100,
				(uLong)file_info.tmu_date.tm_hour,(uLong)file_info.tmu_date.tm_min,
				(uLong)file_info.crc,filename_inzip);
		*/		
				
			           
       s.sp_form=ft_tuple;
       s.sp_val.sp_tuple_ptr =TUPLE_HEADER(cb);
       TUPLE_ADD_CELL(ca,&s);
	
				
		if ((i+1)<gi.number_entry)
		{
			err = unzGoToNextFile(uf);
			if (err!=UNZ_OK)
			{
				giveup(SETL_SYSTEM "error with zipfile in unzGoToNextFile");
				break;
			}
		}
	}
	
   TUPLE_CONSTRUCTOR_END(ca);
   
   unmark_specifier(target); 
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = TUPLE_HEADER(ca);
   return;


}


#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)

/*
  mini unzip, demo of unzip package

  usage :
  Usage : miniunz [-exvlo] file.zip [file_to_extract]

  list the file in the zipfile, and print the content of FILE_ID.ZIP or README.TXT
    if it exists
*/


/* change_file_date : change the date/time of a file
    filename : the filename of the file where date/time must be modified
    dosdate : the new date at the MSDos format (4 bytes)
    tmu_date : the SAME new date at the tm_unz format */
void change_file_date(filename,dosdate,tmu_date)
	const char *filename;
	uLong dosdate;
	tm_unz tmu_date;
{
#ifdef WIN32
  HANDLE hFile;
  FILETIME ftm,ftLocal,ftCreate,ftLastAcc,ftLastWrite;

  hFile = CreateFile(filename,GENERIC_READ | GENERIC_WRITE,
                      0,NULL,OPEN_EXISTING,0,NULL);
  GetFileTime(hFile,&ftCreate,&ftLastAcc,&ftLastWrite);
  DosDateTimeToFileTime((WORD)(dosdate>>16),(WORD)dosdate,&ftLocal);
  LocalFileTimeToFileTime(&ftLocal,&ftm);
  SetFileTime(hFile,&ftm,&ftLastAcc,&ftm);
  CloseHandle(hFile);
#else
#ifdef unix
  struct utimbuf ut;
  struct tm newdate;
  newdate.tm_sec = tmu_date.tm_sec;
  newdate.tm_min=tmu_date.tm_min;
  newdate.tm_hour=tmu_date.tm_hour;
  newdate.tm_mday=tmu_date.tm_mday;
  newdate.tm_mon=tmu_date.tm_mon;
  if (tmu_date.tm_year > 1900)
      newdate.tm_year=tmu_date.tm_year - 1900;
  else
      newdate.tm_year=tmu_date.tm_year ;
  newdate.tm_isdst=-1;

  ut.actime=ut.modtime=mktime(&newdate);
  utime(filename,&ut);
#endif
#endif
}


/* mymkdir and change_file_date are not 100 % portable
   As I don't know well Unix, I wait feedback for the unix portion */

int mymkdir(dirname)
	const char* dirname;
{
    int ret=0;
#ifdef WIN32
	ret = mkdir(dirname);
#else
#ifdef unix
	ret = mkdir (dirname,0775);
#endif
#endif
	return ret;
}

int makedir (newdir)
    char *newdir;
{
#ifdef JUNK
  char *buffer ;
  char *p;
  int  len = strlen(newdir);  

  if (len <= 0) 
    return 0;

  buffer = (char*)malloc(len+1);
  strcpy(buffer,newdir);
  
  if (buffer[len-1] == '/') {
    buffer[len-1] = '\0';
  }
  if (mymkdir(buffer) == 0)
    {
      free(buffer);
      return 1;
    }

  p = buffer+1;
  while (1)
    {
      char hold;

      while(*p && *p != '\\' && *p != '/')
        p++;
      hold = *p;
      *p = 0;
      if ((mymkdir(buffer) == -1) && (errno == ENOENT))
        {
          printf("couldn't create directory %s\n",buffer);
          free(buffer);
          return 0;
        }
      if (hold == 0)
        break;
      *p++ = hold;
    }
  free(buffer);
#endif
  return 1;
}

int do_extract_currentfile(uf,popt_extract_without_path,popt_overwrite,fbuff,ext_or_mem)
	unzFile uf;
	const int* popt_extract_without_path;
    int* popt_overwrite;
    char *fbuff;
    int ext_or_mem;
{
	char filename_inzip[256];
	char* filename_withoutpath;
	char* p;
    int err=UNZ_OK;
    FILE *fout=NULL;
    void* buf;
    uInt size_buf;
	char *s;
	
	unz_file_info file_info;
	uLong ratio=0;
	err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);

	if (err!=UNZ_OK)
	{
		//printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
		return err;
	}

    size_buf = WRITEBUFFERSIZE;
    buf = (void*)malloc(size_buf);
    if (buf==NULL)
    {
        //printf("Error allocating memory\n");
        return UNZ_INTERNALERROR;
    }

	p = filename_withoutpath = filename_inzip;
	while ((*p) != '\0')
	{
		if (((*p)=='/') || ((*p)=='\\'))
			filename_withoutpath = p+1;
		p++;
	}

	if ((*filename_withoutpath)=='\0')
	{
		if ((*popt_extract_without_path)==0)
		{
			//printf("creating directory: %s\n",filename_inzip);
			mymkdir(filename_inzip);
		}
	}
	else
	{
		const char* write_filename;
		int skip=0;

		if ((*popt_extract_without_path)==0)
			write_filename = filename_inzip;
		else
			write_filename = filename_withoutpath;

		err = unzOpenCurrentFile(uf);
		if (err!=UNZ_OK)
		{
			//printf("error %d with zipfile in unzOpenCurrentFile\n",err);
		}

		if (((*popt_overwrite)==0) && (err==UNZ_OK))
		{
			char rep;
			FILE* ftestexist;
			ftestexist = fopen(write_filename,"rb");
			if (ftestexist!=NULL)
			{
				fclose(ftestexist);
				do
				{
					char answer[128];
					//printf("The file %s exist. Overwrite ? [y]es, [n]o, [A]ll: ",write_filename);
					//scanf("%1s",answer);
					rep = 'n'; //answer[0] ;
					if ((rep>='a') && (rep<='z'))
						rep -= 0x20;
				}
				while ((rep!='Y') && (rep!='N') && (rep!='A'));
			}

			if (rep == 'N')
				skip = 1;

			if (rep == 'A')
				*popt_overwrite=1;
		}

		if ((ext_or_mem==1) && (skip==0) && (err==UNZ_OK))
		{
			fout=fopen(write_filename,"wb");

            /* some zipfile don't contain directory alone before file */
            if ((fout==NULL) && ((*popt_extract_without_path)==0) && 
                                (filename_withoutpath!=(char*)filename_inzip))
            {
                char c=*(filename_withoutpath-1);
                *(filename_withoutpath-1)='\0';
                makedir(write_filename);
                *(filename_withoutpath-1)=c;
                fout=fopen(write_filename,"wb");
            }

			if (fout==NULL)
			{
				//printf("error opening %s\n",write_filename);
			}
		}

		if ((ext_or_mem==0) || (fout!=NULL))
		{
			//printf(" extracting: %s\n",write_filename);
			s=fbuff;
			do
			{
				err = unzReadCurrentFile(uf,buf,size_buf);
				if (err<0)	
				{
					//printf("error %d with zipfile in unzReadCurrentFile\n",err);
					break;
				}
				if (err>0)
					if (ext_or_mem==1) {
						if (fwrite(buf,err,1,fout)!=1)
						{
							//printf("error in writing extracted file\n");
	                        err=UNZ_ERRNO;
							break;
						}
					} else { // Memory
						memcpy(s,buf,err);
						s+=err;
					
					}
			}
			while (err>0);
			if (ext_or_mem==1) fclose(fout);
			if (err==0) 
				change_file_date(write_filename,file_info.dosDate,
					             file_info.tmu_date);
		}

        if (err==UNZ_OK)
        {
		    err = unzCloseCurrentFile (uf);
		    if (err!=UNZ_OK)
		    {
			    //printf("error %d with zipfile in unzCloseCurrentFile\n",err);
		    }
        }
        else
            unzCloseCurrentFile(uf); /* don't lose the error */       
	}

    free(buf);    
    return err;
}



SETL_API void EXTRACT_FROM_ZIP(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
char filename_inzip[256];
unz_file_info file_info;
uLong ratio=0;
const char *string_method;

struct setl_zip *A; /* w */ 
int len;
unzFile uf;
TUPLE_CONSTRUCTOR(ca)
TUPLE_CONSTRUCTOR(cb)
STRING_CONSTRUCTOR(sa)
uLong i;
int z,j;
unz_global_info gi;
int err;
STRING_ITERATOR(sc)
int k;
char *str,*filename;

   check_arg(SETL_SYSTEM argv,1,ft_string,"string","extract_from_zip");
 
 if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=zip_type))
       abend(SETL_SYSTEM msg_bad_arg,"zip object",1,"list_zip",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   A = (struct setl_zip *)(argv[0].sp_val.sp_opaque_ptr);
   uf = A->ptr;	
   
   ITERATE_STRING_BEGIN(sc,argv[1]);
   len = STRING_LEN(argv[1]);
   
   
   str=filename=(char*)malloc(len+1);
   if (str==NULL)
   	  giveup(SETL_SYSTEM msg_malloc_error);

 
   ITERATE_STRING_BEGIN(sc,argv[1]);
   for (k=0;k<len;k++) {
      *str++=ITERATE_STRING_CHAR(sc);
      ITERATE_STRING_NEXT(sc);
   }
   *str=0;
    if (unzLocateFile(uf,filename,CASESENSITIVITY)!=UNZ_OK)
    {
      free(filename);	
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return; 
     
    }
   free(filename);
	
	unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
		
    if (do_extract_currentfile(uf,1,1,NULL,1) == UNZ_OK)
        i=0;
    else
        i=1;
    

  
	unmark_specifier(target);
	target->sp_form = ft_short;
	target->sp_val.sp_short_value = i;
    return;


}


SETL_API void GET_FROM_ZIP(
  SETL_SYSTEM_PROTO
  int argc,                           /* number of arguments passed        */
  specifier *argv,                    /* argument vector (two here)        */
  specifier *target)                  /* return value                      */
{
char filename_inzip[256];
unz_file_info file_info;
uLong ratio=0;
const char *string_method;
struct setl_zip *A; /* w */ 
int len;
unzFile uf;
TUPLE_CONSTRUCTOR(ca)
TUPLE_CONSTRUCTOR(cb)
STRING_CONSTRUCTOR(cs)
uLong i;
int z,j;
unz_global_info gi;
int err;
STRING_ITERATOR(sc)
int k;
char *str,*filename;
char *buff;

   check_arg(SETL_SYSTEM argv,1,ft_string,"string","get_from_zip");
 
 if ((argv[0].sp_form != ft_opaque)||
       (((argv[0].sp_val.sp_opaque_ptr->type)&65535)!=zip_type))
       abend(SETL_SYSTEM msg_bad_arg,"zip object",1,"list_zip",
         abend_opnd_str(SETL_SYSTEM argv+0));

 

   A = (struct setl_zip *)(argv[0].sp_val.sp_opaque_ptr);
   uf = A->ptr;	
   
   ITERATE_STRING_BEGIN(sc,argv[1]);
   len = STRING_LEN(argv[1]);
   
   
   str=filename=(char*)malloc(len+1);
   if (str==NULL)
   	  giveup(SETL_SYSTEM msg_malloc_error);

 
   ITERATE_STRING_BEGIN(sc,argv[1]);
   for (k=0;k<len;k++) {
      *str++=ITERATE_STRING_CHAR(sc);
      ITERATE_STRING_NEXT(sc);
   }
   *str=0;
    if (unzLocateFile(uf,filename,CASESENSITIVITY)!=UNZ_OK)
    {
      free(filename);	
      unmark_specifier(target);
      target->sp_form = ft_omega;
      return; 
     
    }
   free(filename);
	
	unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
		
   buff=(char*)malloc(file_info.uncompressed_size+1);
   if (buff==NULL)
   	  giveup(SETL_SYSTEM msg_malloc_error);


    if (do_extract_currentfile(uf,1,1,buff,0) != UNZ_OK) {
    	free(buff);
    	unmark_specifier(target);
        target->sp_form = ft_omega;
        return; 
    
    }
        
   STRING_CONSTRUCTOR_BEGIN(cs);

   i=0;
   str=buff;
   while (i<file_info.uncompressed_size) {
      
        STRING_CONSTRUCTOR_ADD(cs,*str++);
        i++;
   }
   free(buff);

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = STRING_HEADER(cs);
   return;



}

 
